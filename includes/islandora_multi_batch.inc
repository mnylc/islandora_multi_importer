<?php

/**
 * @file
 * A batch ingest implement, (preprocessor and object) for Multi importer.
 *
 * Works similar to other importers derived from islandora_importer module.
 * @author Diego Pino Navarro
 */

/**
 * A class that uses Twig templates and tabulated data to build objects.
 */
class IslandoraMultiBatch extends IslandoraBatchPreprocessor {

  protected $itemClass = "IslandoraMultiBatchObject";

  /**
   * Keeps track of success and failed objects in set.
   *
   * @var processedObjects
   *   An array of all processed Objects.
   */
  protected $processedObjectsInfo = array();

  /**
   * We need PIDS generated on getingestinfo.
   *
   * @var internal_pidcache
   */
  protected static $internalPidcache = array();

  /**
   * Accessor for the Proccessed Objects lists.
   */
  public function getProcessedObjectsInfo() {
    return $this->processedObjectsInfo;
  }

  /**
   * Static function to get PIDs.
   *
   * @param string $namespace
   *   A valid fedora pid namespace.
   * @param int $cachesize
   *   Size of the temp cache for identifiers for this
   *    namespace.
   *
   * @return string
   *   a PID for this namespace.
   */
  protected function getInternalIdentifier($namespace, $cachesize) {
    module_load_include('inc', 'islandora', 'includes/utilities');
    if (empty(self::$internalPidcache[$namespace])) {
      // Get enough PIDs given an initial $cachesize
      // (plus one, so we'll always get at least one).
      // Instead of using the build in parent function
      // use the Islandora API to get the identifier using de uuid variable.
      self::$internalPidcache[$namespace] = (array) $this->connection->repository->getNextIdentifier($namespace, variable_get('islandora_basic_collection_generate_uuid', FALSE), $cachesize + 1);
    }

    return array_shift(self::$internalPidcache[$namespace]);
  }

  /**
   * Try to get the namespace from the collection policy.
   *
   * @param string $parent_pid
   *   A PID.
   * @param string $cmodel
   *   A CMODEL.
   *
   * @return string|null
   *   The namespace with which the object should be ingested.
   *   NULL if we use the Fedora configured default.
   */
  protected function determineNamespacebyPid($parent_pid, $cmodel) {
    // Get namespace based on collection.
    $collection_policies =& drupal_static(__METHOD__, array());
    // Parameter not set.
    $namespace = NULL;
    if (module_exists('islandora_basic_collection') && !isset($collection_policies[$parent_pid])) {
      $parent = islandora_object_load($parent_pid);
      if (isset($parent['COLLECTION_POLICY'])) {
        $policy_stream = $parent['COLLECTION_POLICY'];
        $collection_policies[$parent_pid] = new CollectionPolicy($policy_stream->content);
      }
      else {
        // If not collection, just return the namespace of the
        // Parent PID, assuming they will inherit, like pages, etc?
        // @TODO maybe we need a recursive namespace thing until we hit a collection?
        $namespace_tmp = explode(':', $parent_pid);
        $namespace = trim($namespace_tmp[0]);
      }
    }
    if (isset($collection_policies[$parent_pid])) {
      $info = $collection_policies[$parent_pid]->getContentModels();
      $applicable = array_intersect_key($info, drupal_map_assoc(array($cmodel)));
      if ($applicable) {
        $first_applicable = reset($applicable);
        $namespace = $first_applicable['namespace'];
      }
      else {
        $namespace = "islandora";
      }
    }
    return $namespace;
  }

  /**
   * Try to get the cmodels from the parent objet.
   *
   * @param string $parent_pid
   *   A PID.
   *
   * @return string|null
   *   The namespace with which the object should be ingested.
   *   NULL if we use the Fedora configured default.
   */
  protected function getParentCmodel($parent_pid) {
    // Get namespace based on collection.
    $cmodels =& drupal_static(__METHOD__, array());
    // Parameter not set.
    $current_cmodel = NULL;
    $parent = islandora_object_load($parent_pid);
    if ($parent) {
      $cmodels[$parent_pid] = $parent->models;
    }
    else {
      // Means the object does not exist. So error.
      $cmodels[$parent_pid] = FALSE;
    }

    if (isset($cmodels[$parent_pid])) {
      $current_cmodel = $cmodels[$parent_pid];
    }
    return $current_cmodel;
  }

  /**
   * Get the name of the class to instantiate for the batch operations.
   */
  protected static function getObjectClass() {
    return "IslandoraMultiBatchObject";
  }

  /**
   * Perform preprocessing of the scanned resources.
   */
  public function preprocess() {
    $added = array();
    $object_class = static::getObjectClass();
    $info = $this->getIngestInfo();
    foreach ($info as $index => $object_info) {
      $ingest_object = new $object_class($this->connection, 'New Object via Multi importer', $object_info, $this->parameters);
      $ingest_object->id = $object_info['pid'];
      try {
        $ingest_object->addRelationships();
        $this->addToDatabase($ingest_object, $ingest_object->getResources(), $object_info['parent']);
        $added[] = $ingest_object;
      }
      catch (Exception $e) {
        watchdog_exception('islandora_batch', $e);
      }
    }

    return $added;
  }

  /**
   * {@inheritdoc}
   */
  protected function getIngestInfo() {
    error_log('running getIngestInfo');
    $file = file_load($this->parameters['source_data']);
    $file_path = drupal_realpath($file->uri);
    $file_data_all = islandora_multi_importer_read_filedata($file_path, -1, $offset = 0);
    $this->parameters['data']['headers'] = $file_data_all['headers'];

    if ($this->parameters['type'] == 'ZIP') {
      // We don't need to scan the whole zip file,
      // We will try to just fetch the filename comming from the data
      // during datastream creation.
      $zipfile = file_load($this->parameters['source_binaries']);
      $this->parameters['zipfile'] = drupal_realpath($zipfile->uri);
    }

    $namespace_hash = array();
    // Keeps track of all parents and child that don't have a PID assigned.
    $parent_hash = array();
    $namespace_count = array();
    $info = array();
    // Keeps track of invalid rows.
    $invalid = array();
    foreach ($file_data_all['data'] as $index => $row) {
      // Each row will be an object.
      $objectInfo = array();
      $objectInfo['cmodel'] = trim($row[$this->parameters['cmodel_source_field_index']]);
      // Lets start by grouping by parents, namespaces and generate pids
      // namespaces are inherited, so we just need to find collection
      // objects in parent pid column.
      $objectInfo['parent'] = trim($row[$this->parameters['object_maping']['parentmap_row']['parentmap']]);
      $objectInfo['data'] = $row;
      if (islandora_is_valid_pid($objectInfo['parent'])) {
        // If valid PID, let's try to fetch a valid namespace for current cmodel
        // we will store all this stuff in a temp hash to avoid hitting
        // this again and again.
        $objectInfo['parent_cmodel'] = $this->getParentCmodel($objectInfo['parent']);
        if ($objectInfo['parent_cmodel']) {
          $namespace = isset($namespace_hash[$objectInfo['parent']][$objectInfo['cmodel']])
            ? $namespace_hash[$objectInfo['parent']][$objectInfo['cmodel']]
            : $namespace_hash[$objectInfo['parent']][$objectInfo['cmodel']] = $this->determineNamespacebyPid($objectInfo['parent'], $objectInfo['cmodel']);
          // This keeps the count so our pidcache is exact. Nice.
          $namespace_count[$namespace] = isset($namespace_hash[$namespace]) ? $namespace_hash[$namespace]++ : 1;
          $objectInfo['namespace'] = $namespace;
        }
        else {
          // No parent cmodel, no object, can't create.
          unset($objectInfo);
          $invalid = $invalid + array($index => $index);
        }
      }
      else {
        // Means our parent object is a ROW index
        // (referencing another row in the spreadsheet)
        // So a different strategy is needed. We will need recurse
        // until we find a non numeric parent.
        $notPID = TRUE;
        $parent = $objectInfo['parent'];
        $parent_hash[$parent][$index] = $index;
        $parentchilds = array();
        if ((!isset($invalid[$index])) && (!isset($invalid[$parent]))) {
          // Only traverse if we don't have this index or the parent one
          // in the invalid register.
          $objectInfo['parent_cmodel'] = $file_data_all['data'][$parent][$this->parameters['cmodel_source_field_index']];
          $parentchilds = array();
          $i = 0;
          while ($notPID) {
            $parentup = $file_data_all['data'][$parent][$this->parameters['object_maping']['parentmap_row']['parentmap']];

            // Probably the simples approach for breaking a knott
            // a.k.a infinite loop, invalidating the whole parentship
            // chain for good.
            $inaloop = array_key_exists($parentup, $parentchilds);
            // If $inaloop === true means we already traversed this branch
            // so we are in a loop and all our original child and it's
            // parent objects are invalid.
            if ($inaloop) {
              $invalid = $invalid + $parentchilds;
              unset($objectInfo);
              $notPID = FALSE;
              break;
            }

            $parentchilds[$parentup] = $parentup;
            if (islandora_is_valid_pid(trim($parentup))) {
              $namespace = isset($namespace_hash[$parentup][$objectInfo['cmodel']])
                ? $namespace_hash[$parentup][$objectInfo['cmodel']]
                : $namespace_hash[$parentup][$objectInfo['cmodel']] = $this->determineNamespacebyPid($parentup, $objectInfo['cmodel']
              );
              // This keeps the count so our pidcache is exact. Nice.
              $namespace_count[$namespace] = isset($namespace_hash[$namespace]) ? $namespace_hash[$namespace]++ : 1;
              $objectInfo['namespace'] = $namespace;
              $notPID = FALSE;
              break;
            }
            elseif (empty(trim($parent))) {

              // We can't continue here
              // means there must be an error
              // This will fail for any child object that is
              // child of any of these parents.
              $invalid = $invalid + $parentchilds + array($objectInfo['parent'] => $objectInfo['parent']);
              unset($objectInfo);
              $notPID = FALSE;
            }
            else {
              // This a simple accumulator, means all is well,
              // parent is still an index.
              $parent_hash[$parentup][$parent] = $parent;
            }
            $parent = $parentup;
          }
        }
        else {
          unset($objectInfo);
        }
      }

      if (isset($objectInfo) and !empty($objectInfo)) {
        $info[$index] = $objectInfo;
      }
    }
    // Ok, maybe this is expensive, so let's try it first so.
    // TODO: optimize maybe?
    foreach ($info as $index => &$objectInfo) {
      $namespace = $objectInfo['namespace'];
      $objectInfo['pid'] = isset($objectInfo['pid']) ? isset($objectInfo['pid']) : $this->getInternalIdentifier($namespace, $namespace_count[$namespace]);
      // Is this object parent of someone?
      if (isset($parent_hash[$objectInfo['parent']])) {
        $objectInfo['parent'] = $info[$objectInfo['parent']]['pid'];
      }
    }
    // Keep track of what could be processed and which ones not.
    $this->processedObjectsInfo = array(
      'success' => array_keys($info),
      'error' => array_keys($invalid),
    );

    return $info;
  }

}
/**
 * The Batch Object class.
 */
class IslandoraMultiBatchObject extends IslandoraBatchObject {
  protected $baseName;
  protected $objectInfo;
  /**
   * Keys in objectInfo that are not to be datastreams.
   *
   * @var preprocessorParameters
   */
  protected $preprocessorParameters;
  /**
   * Path to MODS->DC XSLT.
   *
   * @var modsToDcTransform
   */
  public $modsToDcTransform;
  public $deriveDC;

  /**
   * Constructor for the IslandoraScanBatchObject.
   */
  public function __construct(IslandoraTuque $connection, $base_name, $object_info, $preprocessor_parameters) {
    parent::__construct(NULL, $connection->repository);
    // Checks if DC is set as derivative, if so,
    // we can safely let the derivative process deal with this.
    $this->deriveDC = !variable_get('xml_form_builder_use_default_dc_xslts', FALSE);
    $this->modsToDcTransform = drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl';
    $this->baseName = $base_name;
    $this->objectInfo = $object_info;
    $this->resources = array();
    $this->preprocessorParameters = $preprocessor_parameters;
  }

  /**
   * Function batch_process.
   */
  public function batchProcess() {
    // Use object_info to create some datastreams.
    $this->label = !empty($this->getTitle()) ? $this->getTitle() : "New Object";
    $errors = array();
    $files = array();
    $datastreams = array();
    $datastreams = $this->getDatastreams($errors, $files);
    foreach ($files as $file) {
      $file->status &= ~FILE_STATUS_PERMANENT;
      file_save($file);
      file_usage_add($file, 'islandora_batch', 'islandora_batch_object', $this->getBatchId());
    }

    foreach ($datastreams as $ds) {
      $ds_uri = isset($ds['file']) ?
        $ds['file']->uri :
        $ds['datastream_file'];

      $datastream = $this->constructDatastream($ds['dsid'], $ds['control_group']);
      $datastream->label = $ds['label'];
      $datastream->mimetype = $ds['mimetype'];
      error_log($ds_uri);
      $datastream->setContentFromFile($ds_uri);
      $this->ingestDatastream($datastream);
    }
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Acquire base datastreams structure.
   *
   * @see IslandoraImportObject::getDatastreams()
   * Datastreams can be build using multiple sources, so we deal here with that
   */
  protected function getDatastreams(&$errors = NULL, &$files = NULL) {
    module_load_include('inc', 'islandora', 'includes/utilities');
    module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');

    $datastreams = array();

    if (isset($this->preprocessorParameters['cmodel_mapping'][$this->objectInfo['cmodel']])) {
      foreach ($this->preprocessorParameters['cmodel_mapping'][$this->objectInfo['cmodel']]['dsid']['rows'] as $dsid => $howtogenerate) {
        if (!empty($howtogenerate['method']['data'])) {
          $method = explode("|", $howtogenerate['method']['data']);
          switch ($method[0]) {
            case 'derivative':
              // Nothing to do really, derivative processing will do the job.
              break;

            case 'xslt':
              // Deal with xslt transforms, we will leave this to after
              // processing, since we need the source datastreams to transform.
              break;

            case 'template':
              $twig_input = array();
              $template = islandora_multi_importer_twig_fetch($method[1]);
              $twig_input['name'] = $method[1];
              $twig_input['template'] = $template;
              $twig_input['data'] = array('data' => array_combine($this->preprocessorParameters['data']['headers'], $this->objectInfo['data']));
              // @TODO should we validate the template again here?
              $output = islandora_multi_importer_twig_process($twig_input);
              // We need to make sure that $output is in fact an valid XML file.
              $newfileName = "{$dsid}" . "_" . $method[0] . '_' . urlencode($this->objectInfo['pid']) . '_' . time() . '.xml';
              // $tmp_directory = islandora_multi_importer_temp_directory();
              $newfile = file_save_data($output, "public://" . $newfileName, FILE_EXISTS_RENAME);
              // We are assumming the twig template complies with
              // mimetype. TODO enforce this somehow.
              $file[] = $newfile;
              $datastreams[$dsid] = array(
                'dsid' => $dsid,
                'label' => "$dsid datastream",
                'mimetype' => $this->preprocessorParameters['computed_cmodels'][$this->objectInfo['cmodel']][$dsid]['mime'][0],
                'datastream_file' => drupal_realpath($newfile->uri),
                'filename' => $newfileName,
                'control_group' => 'M',
              // 'file' => $file,.
              );
              break;

            case 'column':
              // This means our source field contains a file path or and url
              // if file path, we need to know if local of ZIP
              // if URL just fetch the damn thing.
              if ($this->preprocessorParameters['type'] == 'ZIP') {
                // Fetch one file from provided ZIP file.
                $path_info = pathinfo(trim($this->objectInfo['data'][$method[1]]));
                $z = new ZipArchive();
                error_log($this->preprocessorParameters['zipfile']);
                if ($z->open($this->preprocessorParameters['zipfile'])) {
                  $fp = $z->getStream($this->objectInfo['data'][$method[1]]);
                  if (!$fp) {
                    error_log('not found for ' . $this->id);
                    // The file does not exist.
                    // @TODO add this to errors
                    // try to get it now from remote
                    error_log('will try to get '.$this->objectInfo['data'][$method[1]]. 'remotely');
                    $fileurl = islandora_multi_importer_remote_file_get(trim($this->objectInfo['data'][$method[1]]));
                    if ($fileurl) {
                      $datastreams[$dsid] = array(
                        'dsid' => $dsid,
                        'label' => "$dsid datastream",
                        'mimetype' => $this->getMimetype(drupal_basename($fileurl)),
                        'datastream_file' => $fileurl,
                        'filename' => drupal_basename($fileurl),
                        'control_group' => 'M',
                      // 'file' => $file,.
                      );
                    break;
                    }
                  }
                  $datastreams[$dsid] = array(
                    'dsid' => $dsid,
                    'label' => "$dsid datastream",
                    'mimetype' => $this->getMimetype($path_info['basename']),
                    'datastream_file' => "zip://" . $this->preprocessorParameters['zipfile'] . "#" . $this->objectInfo['data'][$method[1]],
                    'filename' => $path_info['basename'],
                    'control_group' => 'M',
                  );
                  // $z->close();
                }
                else {
                  error_log('can not open ' . $this->preprocessorParameters['zipfile']);
                }
              }
              else {
                // This will handle all cases, except ZIP.
                $fileurl = islandora_multi_importer_remote_file_get(trim($this->objectInfo['data'][$method[1]]));
                if ($fileurl) {
                  $datastreams[$dsid] = array(
                    'dsid' => $dsid,
                    'label' => "$dsid datastream",
                    'mimetype' => $this->getMimetype(drupal_basename($fileurl)),
                    'datastream_file' => $fileurl,
                    'filename' => drupal_basename($fileurl),
                    'control_group' => 'M',
                  // 'file' => $file,.
                  );
                }
              }
              break;

            default:
              break;

          }; // End Switch
        }
      }
    }
    return $datastreams;
  }

  /**
   * Determine the Mimetype for the given file name.
   *
   * @param string $name
   *   A filename for which to determine a mimetype.
   *
   * @return string
   *   - A string containing the detected mimetype of the file.
   */
  protected function getMimetype($name) {

    static $mime_detect = NULL;
    static $dsids = NULL;

    if ($mime_detect === NULL) {
      $mime_detect = new MimeDetect();
    }

    $mimetype = $mime_detect->getMimetype($name);
    return $mimetype;
  }

  /**
   * Get resources for the current object.
   */
  public function getResources() {
    return $this->resources;
  }

  /**
   * Get the title for this object.
   *
   * Gets the label not empty; otherwise, grabs a title from the MODS.
   */
  protected function getTitle() {
    return trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['labelmap_row']['labelmap']]);
  }

  /**
   * Run an XSLT, and return the results.
   *
   * @param array $info
   *   An associative array of parameters, containing:
   *   - input: The input XML in a string.
   *   - xsl: The path to an XSLT file.
   *   - php_functions: Either a string containing one or an array containing
   *     any number of functions to register with the XSLT processor.
   *
   * @return string
   *   The transformed XML, as a string.
   */
  protected static function runXslTransform(array $info) {
    $xsl = new DOMDocument();
    $xsl->load($info['xsl']);

    $input = new DOMDocument();
    $input->loadXML($info['input']);

    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);

    if (isset($info['php_functions'])) {
      $processor->registerPHPFunctions($info['php_functions']);
    }

    // XXX: Suppressing warnings regarding unregistered prefixes.
    return $processor->transformToXML($input);
  }

  /**
   * Function to get dc.
   */
  protected function getDc() {
    if (!isset($this['DC'])) {
      $ingest_dc = FALSE;
      // TODO: Create the DC datastream on this object.
      $dc_datastream = $this->constructDatastream('DC', 'M');
      $dc_datastream->mimetype = 'application/xml';
      $dc_datastream->label = 'DC Record';

      if (isset($this->objectInfo['xml'])) {
        $xml = file_get_contents($this->objectInfo['xml']->uri);

        if ($xml) {
          $s_xml = new SimpleXMLElement($xml);
          if ($s_xml->getName() == 'dc') {
            // XXX: Should probably create a resource entry.
            $dc_datastream->setContentFromFile($this->objectInfo['xml']->uri, FALSE);
            $ingest_dc = TRUE;
          }
        }
      }

      // Get the DC by transforming from MODS.
      if (!isset($dc_datastream->content)) {
        // XXX: Might want to make this use a file, instead of staging the DC
        // in the database table (inside the object we serialize).
        $mods_content = $this->getMods();
        if ($mods_content) {
          $new_dc = static::runXslTransform(
            array(
              'xsl' => $this->modsToDcTransform,
              'input' => $mods_content,
            )
          );
        }
        if (isset($new_dc)) {
          $ingest_dc = TRUE;
          $dc_datastream->content = $new_dc;
        }
      }
      if ($ingest_dc) {
        $this->ingestDatastream($dc_datastream);
      }
    }

    return isset($this['DC']) ? $this['DC']->content : FALSE;
  }

  /**
   * Fetch a valid parent RELS-EXT relationship for cmodel pair.
   */
  protected function setValidRelationships() {
    // TODO this should make use of Ontology Solution Pack instead
    // of hardcoding this mess.
    $parent_pid = $this->objectInfo['parent'];
    if (islandora_is_valid_pid($parent_pid)) {
      // I need to know what CMODEL the parent has.
      // This is pre-calculated by the preprocessor.
      $parent_cmodel = (array) $this->objectInfo['parent_cmodel'];
      // We can only get one for now.
      switch (reset($parent_cmodel)) {
        case 'islandora:bookCModel':
        case 'islandora:newspaperIssueCModel':
          $this->setValidPagedRelationships($parent_pid);
          break;

        case 'islandora:newspaperCModel':
          $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOf', $parent_pid);
          $this->relationships->add(
          ISLANDORA_RELS_EXT_URI,
          'isSequenceNumber',
          trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]),
          RELS_TYPE_PLAIN_LITERAL
          );
        case 'islandora:collectionCModel':
          $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $parent_pid);
          break;

        case 'islandora:compoundCModel':
          // Compund or any other sine compound allows also free linking.
        default:
          $escaped_pid = str_replace(':', '_', $parent_pid);
          $rels_predicate = variable_get('islandora_compound_object_relationship', 'isConstituentOf');
          $this->relationships->add(FEDORA_RELS_EXT_URI, $rels_predicate, $parent_pid);
          $this->relationships->add(
          ISLANDORA_RELS_EXT_URI,
          "isSequenceNumberOf$escaped_pid",
          trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]),
          RELS_TYPE_PLAIN_LITERAL
          );
          break;
      }
    }
  }

  /**
   * Sets valid paged solutionpack predicates into RELS-EXT.
   */
  protected function setValidPagedRelationships($parent_pid) {
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isPageOf', $parent_pid);
    $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOf', $parent_pid);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]), RELS_TYPE_PLAIN_LITERAL);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isPageNumber', trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]), RELS_TYPE_PLAIN_LITERAL);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSection', '1', RELS_TYPE_PLAIN_LITERAL);
    // @TODO fetch the language from somewhere?
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'hasLanguage', 'eng', RELS_TYPE_PLAIN_LITERAL);
  }

  /**
   * Add collection and content model relationships.
   */
  public function addRelationships() {
    $this->setValidRelationships();
    $this->addContentModelRelationships();
    $this->inheritXacmlPolicies();
  }

  /**
   * Add inheritXacmlFrom relationship.
   */
  protected function inheritXacmlPolicies() {
    if (module_exists('islandora_xacml_editor')) {
      $collection = $this->objectInfo['parent'];
      $parent = islandora_object_load($collection);
      if ($parent) {
        islandora_xacml_editor_apply_parent_policy($this, $parent);
      }
    }
  }

  /**
   * Add the content model relationship(s).
   */
  protected function addContentModelRelationships() {
    $this->models = $this->objectInfo['cmodel'];
  }

}
