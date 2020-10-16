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
   * Perform the actual preprocessing of the resources.
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

  protected function getPdfInfo($fileasstring) {
    module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
    $pages = FALSE;
    $errors = [];
    $fileurl = $this->getMeAFile($fileasstring, $errors);
    foreach($errors as $error) {
      drupal_set_message($error, 'warning');
    }
    if ($fileurl) {
      try {
        $pages = islandora_paged_content_length_of_pdf($fileurl);
      }
      catch (Exception $exception) {
        drupal_set_message(t('We could not get the number of pages from @file needed to generate the objects', ['@file' => $fileasstring]), 'warning');

      }
    }
    return $pages;
  }


  protected function getMeAFile($fileasstring, array &$errors = []){

    $path_info = pathinfo(str_replace(" ", "\x20", trim($fileasstring)));
    $filename = $path_info['filename'];

    if (isset($this->parameters['zipfile']) && !empty($this->parameters['zipfile'])) {
      // Fetch one file from provided ZIP file.
      $z = new ZipArchive();
      $opened = FALSE;
      $opened = $z->open($this->parameters['zipfile']);
      if ($opened === TRUE) {
        $fp = $z->getStream($fileasstring);
        if (!$fp) {
          $errors[] = t('We could not find a needed source %file file in the expected location, will try to get it remotely',
            array(
              '%filename' => trim($fileasstring),
            ));
          // The file does not exist.
          // try to get it now from remote
          $fileurl = islandora_multi_importer_remote_file_get(trim($fileasstring));
           if ($fileurl) {
             $z->close();
             // We can really not trust mime type in this scenario
            return $fileurl;
          }
          else {
            $errors[] = t('We could not find a needed source %file file no where, so giving up',
              array(
                '%filename' => trim($fileasstring),
              ));
            $z->close();
            return FALSE;

          }
        }
        else {
          // This means we have the file inside the ZIP.
          // Deal with non extensions when inside a ZIP
          $tmp_directory = islandora_multi_importer_temp_directory();
          $fileurl = "zip://" . str_replace(" ", "\x20", $this->parameters['zipfile']) . "#" . trim($fileasstring);
          $destination = $tmp_directory.'/'.md5($fileurl).'.'.$path_info['extension'];
          if (!file_exists($destination)) {
            $success = file_put_contents($destination, $fp);
          }
          error_log($fileurl);
          error_log($destination);

          if ($success) {
            $z->close();
            return $destination;
          }
        }
        $z->close();
      }
      else {
        $errors[] = t('Provided ZIP file %zipfile can not be opened or is invalid. Code %errorcode.',
          array(
            '%zipfile' => $this->preprocessorParameters['zipfile'],
            '%errorcode' => (int)$opened,
          ));
        return FALSE;
      }
    }
    // This will handle all cases, except ZIP.
    else {
      $fileurl = islandora_multi_importer_remote_file_get(trim($fileasstring));
      if ($fileurl) {
        return $fileurl;
      }
      else {

        $errors[] = t('We tried but we could not find a needed source %filename file was nowhere to be found. giving up',
          array(
            '%filename' => trim($fileasstring),
          ));
        return FALSE;
      }
    }
  }




  /**
   * {@inheritdoc}
   */
  protected function getIngestInfo() {
    error_log('Running Multi Importer getIngestInfo');
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
      // Lets check this one. So many times everything fails because of a
      // bad Zip file.
      $z = new ZipArchive();
      $opened = FALSE;
      $opened = $z->open($this->parameters['zipfile']);
      $z->close();
      if ($opened !== TRUE) {
        $this->processedObjectsInfo = array(
          'success' => array(),
          'error' => array_keys($file_data_all),
          'fatal' => t('Provided ZIP file %zipfile can not be opened or is invalid. Dropping all preprocessing. Sorry, can not continued. Code %errorcode.',
                        array(
                          '%zipfile' => $this->parameters['zipfile'],
                          '%errorcode' => (int)$opened,
                        )
                      ),
        );
        return array();
        // Means the provided ZIP file can not be read. Gosh!
        // Let's let the user know this
      }
      else {
        // Add this ZIP to the file usage table. That way it will stay even if drupal crons it out.
        $zipfile->status = ~FILE_STATUS_PERMANENT;
        file_save($zipfile);
        file_usage_add($zipfile, 'islandora_batch', 'islandora_batch_set', $this->getSetId());
      }
    }

    $namespace_hash = array();
    // Keeps track of all parents and child that don't have a PID assigned.
    $parent_hash = array();

    // October 2020: Keeps track of all children that need OBJs from parents

    $special_children_hash = array();

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
      $possiblePID = "";
      
      $objectInfo['data'] = $row;

      // October 2020: Check if any of the rows/future objects needs to use the parent OBJ as source
      // Right now we only accept PDFs in a Book or Newspaper
      // Pages (1 or more) will have a special "parent OBJ" mapping
      // But we do not need to check the CMODEL since the UI limits who can already
      if (isset($this->parameters['cmodel_mapping'][$objectInfo['cmodel']]['dsid']['rows']['OBJ'])) {
        $howtogenerate = $this->parameters['cmodel_mapping'][$objectInfo['cmodel']]['dsid']['rows']['OBJ'];
        // We do not have access to the parent yet here. So we just flag it
        if (strpos($howtogenerate['method']['data'], "parent_object|") === 0 ) {
        // Means we got os a cross dependant Page to Parent OBJ case
        // BUT. To allow also books that have their own! images in OBJ present
         $method = explode("|", $howtogenerate['method']['data']);
         $parent_dsid = $method[1];
          // WE will do an extra check. The actual parent mapped column needs to be empty
          // In the current object. If not treat as a normal page
          // Further down we will also do extra checks!
          if (strlen(trim($objectInfo['data'][$parent_dsid])) == 0) {
            $objectInfo['parent_object_obj'] = $parent_dsid;
            $special_children_hash[$index] = $index;
          }
       }
      }



      // Autogeneration of PID is enabled and a PID column is provided
      // both modes complement/limit each other
      // in case of update or any other action that is npt ingest, always check for existing PID
      if (($this->parameters['object_maping']['pidmap_row']['pidtype']) != 1 || $this->parameters['action']!= 'ingest') {
        // User provided PIDs are not required to respect collection policies, namespace, etc.
        $possiblePID = trim($row[$this->parameters['object_maping']['pidmap_row']['pidmap']]);
        if (!empty($possiblePID)) {
          if (islandora_is_valid_pid($possiblePID)) {
            $objectInfo['pid'] = $possiblePID;
            // Now be more strict for action = update
            if ($this->parameters['action'] == 'update') {
              $existing_object = islandora_object_load($possiblePID);
              if (!$existing_object) {
                unset($objectInfo);
                $invalid = $invalid + array($index => $index);
              }
            }  
          }
        }
        if (!isset($objectInfo['pid'])) {
          unset($objectInfo);
          $invalid = $invalid + array($index => $index);
        }
      }  
      // NO else statement because autogeneration is in fact the standard
      // QUESTION IS: NO AUTOGENERATION MODE, COLUMN IS SET, PID IS EMPTY
      // SHOULD WE TRY TO GENERATE IT AUTOMATICALLY ANYWAY?
      
      // Could happen that we already invalidated this row, so check if $objectInfo is
      // is still around. New since allowing user provided PID.
      if (isset($objectInfo)) {
        if (islandora_is_valid_pid($objectInfo['parent'])) {
        // If valid PID, let's try to fetch a valid namespace for current cmodel
        // we will store all this stuff in a temp hash to avoid hitting
        // this again and again.
          $objectInfo['parent_cmodel'] = $this->getParentCmodel($objectInfo['parent']);
          if ($objectInfo['parent_cmodel']) {
            if (!isset($objectInfo['pid'])) { //Only do this if no PID assigned yet
              $namespace = isset($namespace_hash[$objectInfo['parent']][$objectInfo['cmodel']])
                ? $namespace_hash[$objectInfo['parent']][$objectInfo['cmodel']]
                  : $namespace_hash[$objectInfo['parent']][$objectInfo['cmodel']] = $this->determineNamespacebyPid($objectInfo['parent'], $objectInfo['cmodel']);
              // This keeps the count so our pidcache is exact. Nice.
              $namespace_count[$namespace] = isset($namespace_hash[$namespace]) ? $namespace_hash[$namespace]++ : 1;
              $objectInfo['namespace'] = $namespace;
            }
            else {
              // we have a PID but i still want my objectInfo['namespace']
              // NO worries about checking if pidparts is in fact length of 2
              // PID was checked for sanity a little bit earlier
              $pidparts = explode(":", $objectInfo['pid']);
              $objectInfo['namespace'] = $pidparts[0];
            }
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
          // Lets check if the index actually exists before going crazy.
          
          if (!isset($file_data_all['data'][$parent])) {
            $invalid[$parent] = $parent;
            $invalid[$index] = $index;
          }
          
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
              $inaloop = isset($parentchilds[$parentup]);
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
                if (!isset($objectInfo['pid'])) { //Only do this if no PID assigned yet
                  $namespace = isset($namespace_hash[$parentup][$objectInfo['cmodel']])
                    ? $namespace_hash[$parentup][$objectInfo['cmodel']]
                    : $namespace_hash[$parentup][$objectInfo['cmodel']] = $this->determineNamespacebyPid($parentup, $objectInfo['cmodel']
                  );
                  // This keeps the count so our pidcache is exact. Nice.
                  $namespace_count[$namespace] = isset($namespace_hash[$namespace]) ? $namespace_hash[$namespace]++ : 1;
                  $objectInfo['namespace'] = $namespace;
                } 
                else {
                  // we have a PID but i still want my objectInfo['namespace']
                  // NO worries about checking if pidparts is in fact lenght of 2
                  // PID was checked for sanity a little bit earlier
                  $pidparts = explode(":", $objectInfo['pid']);
                  $objectInfo['namespace'] = $pidparts[0];
                }
              
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
      }
      if (isset($objectInfo) and !empty($objectInfo)) {
        $info[$index] = $objectInfo;
      }
    }
    // Ok, maybe this is expensive, so let's try it first so.
    // TODO: optimize maybe?
    
    // New first pass: ensure parents have always a PID first
    // since rows/parent/child order could be arbitrary
    foreach ($parent_hash as $parent => $children) {
      if (isset($info[$parent])) {
        $namespace = $info[$parent]['namespace'];
        $info[$parent]['pid'] = isset($info[$parent]['pid']) ? $info[$parent]['pid'] : $this->getInternalIdentifier($namespace, $namespace_count[$namespace]);
      }
    }

    // Now we need to preprocess Children with Special parent needs
    // In specific ones that are going to Borrow a PDF from the parent's OBJ
    // Should we also clear the Parent's OBJ afterwards?
    foreach ($special_children_hash as $child) {
      dpm($child);

      if (isset($info[$child])) {
        $parent = $info[$child]['parent'];
        $source_datastream = $info[$child]['parent_object_obj'];
        // This is where we copy!
        $info[$child]['data'][$source_datastream] = $info[$parent]['data'][$source_datastream];
        // But that is not enough! Now we need to figure out if we need to generate pages or only use existing ones
        // Two conditions will apply for generating
        // 1.- Only one page present (master to be used as template data)
        // 2.- a Sequence number of eXaCtLY == 0. Easy peasy
        dpm($parent_hash[$parent]);
        dpm($info[$child]['data'][$this->parameters['object_maping']['sequencemap_row']['sequencemap']]);
        dpm($info[$child]['data'][$source_datastream]);

        if (count($parent_hash[$parent]) == 1 &&
          (int) $info[$child]['data'][$this->parameters['object_maping']['sequencemap_row']['sequencemap']] == 0 &&
          strlen(trim($info[$child]['data'][$source_datastream])) > 4
        ) {
          // So 5, is a single character, a dot and the pdf extension or the ps extension
          // Means we need a lot of extra pages based on the PDF info info.
          // GOSH I CREATED A MONSTER!
          $number_of_pages = $this->getPdfInfo($info[$child]['data'][$source_datastream]);
          if ($number_of_pages) {
            // First update sequence number of the first one to 1
            $info[$child]['data'][$this->parameters['object_maping']['sequencemap_row']['sequencemap']] = 1;
            $labelmap = $this->parameters['object_maping']['labelmap_row']['labelmap'];
            $number_of_pages = $number_of_pages - 1; // because yea, maybe someone just got one!
            foreach($number_of_pages as $pagenumber ) {
              $theclone = $info[$child];
              $theclone['data'][$labelmap] =  $theclone['data'][$labelmap] . ' ' . $pagenumber;
              $info[] = $theclone;
            }
            $namespace = $info[$child]['namespace'];
            $namespace_count[$namespace] = $namespace_count[$namespace] + $number_of_pages;
          }
        }
      }
    }


    // Now the real pass, iterate over every row.
    foreach ($info as $index => &$objectInfo) {
      $namespace = $objectInfo['namespace'];
      $objectInfo['pid'] = isset($objectInfo['pid']) ? $objectInfo['pid'] : $this->getInternalIdentifier($namespace, $namespace_count[$namespace]);
      // Is this object parent of someone?
      if (isset($parent_hash[$objectInfo['parent']])) {
        $objectInfo['parent'] = $info[$objectInfo['parent']]['pid'];
      }
    }
    // Keep track of what could be processed and which ones not.
    $this->processedObjectsInfo = array(
      'success' => array_keys($info),
      'error' => array_keys($invalid),
      'fatal' => array(),
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
  protected $currentState;
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
  
  /**
   * If IMI needs to deal with MODS to DC
   *
   * @var $deriveDC
   */
  public $deriveDC;
  
  /**
   * Path to MODS SELF transform XSLT.
   *
   * @var $modsCleanUpTransform
   */
  public $modsCleanUpTransform;
  
  /**
   * If MODS self transform is desired.
   *
   * @var $cleanMODS;
   */
  public $cleanMODS;
    

  /**
   * Constructor for the IslandoraScanBatchObject.
   */
  public function __construct(IslandoraTuque $connection, $base_name, $object_info, $preprocessor_parameters) {
    parent::__construct(NULL, $connection->repository);

    // Checks if DC is set as derivative, if so, we can safely 
    // let the derivative process deal with this. @TODO check per CMODEL too.

    $this->deriveDC = !variable_get('xml_form_builder_use_default_dc_xslts', FALSE);
    $mtdt = drupal_get_path('module', 'islandora_multi_importer') . '/xslt/mods_to_dc.xsl';
    $this->modsToDcTransform = variable_get('islandora_multi_importer_mods_to_dc', $mtdt);
    $mcut = drupal_get_path('module', 'islandora_multi_importer') . '/xslt/islandora_cleanup_mods_extended_strict.xsl';
    $this->modsCleanUpTransform = variable_get('islandora_multi_importer_mods_cleanup', $mcut);
    $this->baseName = $base_name;
    $this->cleanMODS = variable_get('islandora_multi_importer_modsself', FALSE);
    $this->objectInfo = $object_info;
    $this->resources = array();
    $this->preprocessorParameters = $preprocessor_parameters;
    $this->currentState = ISLANDORA_BATCH_STATE__READY;
  }

  /**
   * Function batch_process.
   */
  public function batchProcess() {
    
    if ($this->preprocessorParameters['action'] == 'update') {
      return $this->batchProcessforUpdate(); 
    }
    else {
      return $this->batchProcessforIngest(); 
    }
  }
  /**
   * Function batchProcessforIngest.
   */
  protected function batchProcessforIngest() {
     $setid = $this->getBatchId();
      if ($this->deriveDC) {
          // Do this only once.
          $xsl = new DOMDocument;
          $xsl->load($this->modsToDcTransform);
          if (!$xsl) {
              // Disable transform if this is the case
              $this->deriveDC = FALSE; 
              $params = array('@setid' => $setid, '@pid' => $this->id, '@log' => 'Could not load global XSLT for MODS2DC transformation. MODS to DC disabled!');
              $msg = 'There were some issues on Batch set id @setid when loading DC to MODS via XSLT: @log';
              watchdog('islandora_multi_importer', $msg, $params, WATCHDOG_NOTICE);
          }
      }
      // Use object_info to create some datastreams.
      $this->label = !empty($this->getTitle()) ? $this->getTitle() : "New Object";
      $errors = array();
      $files = array();
      $datastreams = array();
      $datastreams = $this->getDatastreams($errors, $files);
      if (!empty($errors)) {
          foreach ($errors as $error) {
              $params = array('@setid' => $setid, '@pid' => $this->id, '@log' => $error );
              $msg = 'There were some issues on Batch set id @setid when generating datastreams for PID @pid: @log';
              watchdog('islandora_multi_importer', $msg, $params, WATCHDOG_NOTICE);
          }
      }
      if (empty($datastreams)) {
          $this->currentState = ISLANDORA_BATCH_STATE__ERROR;
          return array('message' => 
              t('Serious datastream issues on @pid have forced us to not ingest it.',
          array('@pid' => $this->id)), 'state' => ISLANDORA_BATCH_STATE__ERROR);
      }
    
      foreach ($files as $file) {
          $file->status &= ~FILE_STATUS_PERMANENT; // Lets make all files temporary but trust file_usage to keep them around
          file_save($file);
          file_usage_add($file, 'islandora_batch', 'islandora_batch_object', $this->getBatchId());
      }
      // We need this to check if we have mods
      $dowehaveMODS = false;
      $dowehaveDC = false;
      $dcxml = FALSE;
      foreach ($datastreams as $ds) {
          $ds_uri = isset($ds['file']) ?
              $ds['file']->uri :
          $ds['datastream_file'];

          $datastream = $this->constructDatastream($ds['dsid'], $ds['control_group']);
          $datastream->label = $ds['label'];
          $datastream->mimetype = $ds['mimetype'];
    
          $datastream->setContentFromFile($ds_uri);
          $this->ingestDatastream($datastream);
          $dowehaveMODS = $ds['dsid'] == "MODS" ? true : $dowehaveMODS; 
          $dowehaveDC = $ds['dsid'] == "DC" ? true : $dowehaveDC; 
      }
    
      // Now check if DC to MODS is derivative based or not
      // If DC was provided via file or Twig template don't derive.
      if (($this->deriveDC) && ($dowehaveMODS) && (!$dowehaveDC)) {
          // Not sure if this is more performant than opening the File directly.
          $xml = new DOMDocument;
          $xml->loadXML($this['MODS']->content);
          $proc = new XSLTProcessor;
          $proc->importStyleSheet($xsl);
      
          $dcxml  = $proc->transformToXML($xml);
          if (!$dcxml) {
              $parms = array('@setid' => $setid, '@pid' => $this->id, '@log' => 'Could not derive DC from MODS');
              $msg = 'There were some issues on Batch set id @setid when generating DC datastreams from MODS via XSLT for PID @pid: @log';
              watchdog('islandora_multi_importer', $msg, $parms, WATCHDOG_NOTICE);
          }
          else {
              $datastream = $this->constructDatastream('DC', 'M');
              $datastream->label = 'DC Record';
              $datastream->mimetype = 'application/xml';
              $datastream->setContentFromString($dcxml);
              $this->ingestDatastream($datastream);
          }
      }
      // Collection Policy anyone?
      if ($this->objectInfo['cmodel']=='islandora:collectionCModel' && !isset($this['COLLECTION_POLICY'])) {
          // @TODO: should we construct here one based on the CMODELS ingested? Not sure.
          $cp = $this->constructDatastream('COLLECTION_POLICY', 'M');
          $cp->label = 'Collection Policy';
          $cp->mimetype = 'application/xml';
          $emptypolicy = CollectionPolicy::emptyPolicy();
          $cp->setContentFromString($emptypolicy->getXML());
          $this->ingestDatastream($cp);
      }


      $this->currentState = ISLANDORA_BATCH_STATE__DONE;
      return $this->currentState;
  }
  

  /**
   * Function batchProcessforIngest.
   */
  protected function batchProcessforUpdate() {
    $existing_object = islandora_object_load($this->id);
    if ($existing_object) {
        // Kim said so. No CMODEL transmutation for now.
        // But somehow i managed to enable this?
        // For now, don´t enable this, but we can add a switch?
      if (!in_array($this->objectInfo['cmodel'], $existing_object->models)) {
        $this->currentState = ISLANDORA_BATCH_STATE__ERROR;
        return array('message' => 
          t('Target Object %pid has a different CMODEL than batch provided one, update errored.',
          array('%pid' => $this->id)), 'state' => ISLANDORA_BATCH_STATE__ERROR);
      }
      
      // Use object_info to create/copy some datastreams.
    
      $this->label = !empty($this->getTitle()) ? $existing_object->label = $this->getTitle() : "New Object";
      $errors = array();
      $files = array();
      $datastreams = array();
      $datastreams = $this->getDatastreamsforUpdate($errors, $files, $existing_object);
      if (!empty($errors)) {
        $setid = $this->getBatchId();
        foreach ($errors as $error) {
          $params = array('@setid' => $setid, '@pid' => $this->id, '@log' => $error );
          $msg = 'There were some issues on Batch set id @setid when generating datastreams for PID @pid: @log';
          watchdog('islandora_multi_importer', $msg, $params, WATCHDOG_NOTICE);
        }
      }
      foreach ($files as $file) {
        $file->status &= ~FILE_STATUS_PERMANENT; // Lets make this temporary but trust file_usage to keep them around
        file_save($file);
        file_usage_add($file, 'islandora_batch', 'islandora_batch_object', $this->getBatchId());
      }
      
      foreach ($datastreams as $ds) {
        $ds_uri = isset($ds['file']) ?
          $ds['file']->uri :
          $ds['datastream_file'];
        error_log($ds_uri);
        
        if (!isset($existing_object[$ds['dsid']])) {
          error_log("constructing {$ds['dsid']} for existing {$existing_object->id}");
          $datastream = $this->constructDatastream($ds['dsid'], $ds['control_group']);
          $datastream->label = $ds['label'];
          $datastream->mimetype = $ds['mimetype'];
          
        }
        else {
          $datastream = $existing_object[$ds['dsid']];
          if ($ds['mimetype']!= $datastream->mimetype) {
            $datastream->mimetype = $ds['mimetype'];
          }
          if (!empty($ds['label']) && $datastream->label != $ds['label']) {
            $datastream->label = $ds['label'];
          }
        }
        
        $datastream->setContentFromFile($ds_uri);
        //redundant you would say, but i like to check twice.
        if (!isset($existing_object[$ds['dsid']])) {
          error_log("ingesting {$ds['dsid']} for existing {$existing_object->id}");
          $existing_object->ingestDatastream($datastream);
        }
        //Also, keep a copy in our never to be ingested batchobject
        if (!isset($this[$ds['dsid']])) {
          // But only "ingest" if not already there. 
          // adding relationships to an batch Object
          // also ingests the ds. Obvious!
          $this->ingestDatastream($datastream);
        }
      }

      $this->currentState = ISLANDORA_BATCH_STATE__DONE;
      return ISLANDORA_BATCH_STATE__DONE;
    
    }
    else {
      $this->currentState = ISLANDORA_BATCH_STATE__ERROR;
      return array('message' => t('Target Object %pid does not exists, update errored.', array('%pid' => $this->id)), 'state' => ISLANDORA_BATCH_STATE__ERROR);
    }
  }

  /**
   * The actual back-end action that is run after the batchProcess.
   *
   * By default Classes not overriding this will get objects pushed to the back-end.
   * (ingested) but we are just different here.
   *
   * @return AbstractObject
   */
  public function batchRepositoryAction() {
    
    if ($this->preprocessorParameters['action'] == 'update') {
      // HAHA. Not much. Heavy lifting is done in ::batchProcess()
      return islandora_object_load($this->id);
      
    } 
    else {
      return parent::batchRepositoryAction();
    }
    // need to define a wording for this like "ingest", "update, etc" This needs to return
    // an Array full of little funny data
  }


  /**
   * A human readable short description of the repository action.
   *  Recomended is a verb in past tense.
   *  
   * @return string
   */
  public function getRepositoryActionDescription() {
    if ($this->preprocessorParameters['action'] == 'update') {
      return "updated";
      
    } 
    else {
      return parent::getRepositoryActionDescription();
    }
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
    module_load_include('inc', 'islandora_paged_content', 'includes/utilities');


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
            // @TODO should we validate the template again?
            $twig_output = islandora_multi_importer_twig_process($twig_input);
            // Check if this is can be a DomDocument of some sort.
            $docmimetype = $this->preprocessorParameters['computed_cmodels'][$this->objectInfo['cmodel']][$dsid]['mime'][0];
              
            if (($this->cleanMODS) && ($dsid == 'MODS') && ($twig_output != NULL)) { 
              module_load_include('inc', 'xml_form_builder','includes/associations');
              $twig_output = xml_form_builder_transform_document($this->modsCleanUpTransform, $twig_output);
            }
            // FAIL!
            if ($twig_output == NULL) {
              $errors[] = t('For Object %id, %dsid datastream: twig %template processing had some serious problems. Skipping this object entirely',
              array(
                '%template' => $twig_input['name'],
                '%dsid' => $dsid,
                '%id' => $this->id,
              ));
              $datastreams = array(); // Yeah, we need to roll back all our hard work
              // Aborting, so need to make any files back to temporary 
              foreach ($files as $file) {
                $file->status &= ~FILE_STATUS_PERMANENT; 
                file_save($file);
              }
              unset($files);
              break 2;
            }
            // We need to make sure that $output is in fact a valid XML file.
            $newfileName = "{$dsid}" . "_" . $method[0] . '_' . urlencode($this->objectInfo['pid']) . '_' . time() . '.xml';
            // $tmp_directory = islandora_multi_importer_temp_directory();
            $newfile = file_save_data($twig_output, "public://" . $newfileName, FILE_EXISTS_RENAME);
            // We are assumming the twig template complies with
            // mimetype. TODO enforce this somehow.
            $files[] = $newfile;
            $datastreams[$dsid] = array(
              'dsid' => $dsid,
              'label' => "$dsid datastream",
              'mimetype' => $docmimetype,
              'datastream_file' => drupal_realpath($newfile->uri),
              'filename' => $newfileName,
              'control_group' => 'M',
            );
            break;

            case 'parent_object':
              $resolution = 300;
              $device = isset($this->preprocessorParameters['pdf_device']) ? $this->preprocessorParameters['pdf_device'] : 'tiff24nc';
            // Will deal same as column but with a twist
            case 'column':
            // This means our source field contains a file path or and url
            // if file path, we need to know if local of ZIP
            // if URL just fetch the damn thing.
            $path_info = pathinfo(str_replace(" ", "\x20", trim($this->objectInfo['data'][$method[1]])));
            $filename = $path_info['filename'];
     
            if ($this->preprocessorParameters['type'] == 'ZIP') {
              // Fetch one file from provided ZIP file.
              $z = new ZipArchive();
              $opened = FALSE;
              $opened = $z->open($this->preprocessorParameters['zipfile']);
              if ($opened === TRUE) {
                $fp = $z->getStream($this->objectInfo['data'][$method[1]]);
                error_log($this->objectInfo['data'][$method[1]]);
                if (!$fp) {
                    $errors[] = t('For Object %id, %dsid datastream: We could not find the source %file file in the expected location, will try to get it remotely',
                    array(
                      '%filename' => trim($this->objectInfo['data'][$method[1]]),
                      '%dsid' => $dsid,
                      '%id' => $this->id,
                    ));
                  // The file does not exist.
                  // try to get it now from remote
                  $fileurl = islandora_multi_importer_remote_file_get(trim($this->objectInfo['data'][$method[1]]));
                  $themime = "application/octet-stream";

                  if ($method[0] == 'parent_object' && $fileurl) {
                    $offset = trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]);

                    error_log($this->objectInfo['data'][$method[1]]);
                    error_log($device);
                    error_log($offset);
                    $fileurl = islandora_paged_content_extract_tiff_from_pdf($fileurl, $offset, $device, $resolution);
                    // Get path info agaain for the returned mimetype
                    $path_info = pathinfo(str_replace(" ", "\x20", trim($fileurl)));
                    $themime = "image/tiff";
                  }


                  if ($fileurl) { 
                    $themime = property_exists($fileurl, 'filemime') ? $fileurl->filemime : $this->getMimetype($path_info['basename']);
                    if ($dsid == "HOCR" && $themime == "application/octet-stream") {
                      $themime = "text/html";
                    }
                    if ($dsid == "OCR" && $themime == "application/octet-stream") {
                      $themime = "text/plain";
                    }
                    // Fedora labels are max 255 chars 
                    $dslabel = substr($filename,0,255);
                    //remove any trailing spaces
                    $dslabel = trim($dslabel);
                    $dslabel = !empty($dslabel) ? $dslabel : "$dsid datastream";

                    $datastreams[$dsid] = array(
                      'dsid' => $dsid,
                      'label' => $dslabel,
                      'mimetype' => $themime,
                      'datastream_file' => $fileurl,
                      'filename' => drupal_basename($fileurl),
                      'control_group' => 'M',
                    );
                    break;
                  }
                  else {
                    $errors[] = t('For Object %id, %dsid datastream: %filename was not found inside the provided ZIP file %zipfile nor in other places',
                    array(
                      '%filename' => trim($this->objectInfo['data'][$method[1]]),
                      '%zipfile' => $this->preprocessorParameters['zipfile'],
                      '%dsid' => $dsid,
                      '%id' => $this->id,
                    ));
                  }
                } 
                else {
                  // This means we have the file inside the ZIP.
                  // Deal with non extensions when inside a ZIP
                 
                  $themime = $this->getMimetype($path_info['basename']);
                  $fileurl = "zip://" . str_replace(" ", "\x20", $this->preprocessorParameters['zipfile']) . "#" . trim($this->objectInfo['data'][$method[1]]);

                  if ($method[0] == 'parent_object') {
                    error_log('should be in the ZIP');
                    // islandora paged can not deal with zipped files sadly
                    // we need a temp one
                    $tmp_directory = islandora_multi_importer_temp_directory();
                    $destination = $tmp_directory.'/'.md5($fileurl).'.'.$path_info['extension'];
                    if (!file_exists($destination)) {
                      $success = file_put_contents($destination, $fp);
                    }

                    $offset = trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]);
                    $offset = ($offset > 0 ) ? (int) $offset : 1;
                    error_log($success);
                    error_log($fileurl);
                    error_log($device);
                    error_log($offset);

                    if ($success) {
                      $fileurl = islandora_paged_content_extract_tiff_from_pdf($destination, $offset, $device, $resolution);
                    }
                    error_log($fileurl);
                    // Get path info agaain for the returned mimetype
                    $path_info = pathinfo(str_replace(" ", "\x20", trim($fileurl)));
                    $themime = "image/tiff";
                  }

                  if ($dsid == "HOCR" && $themime == "application/octet-stream") {
                    $themime = "text/html";
                  }
                  if ($dsid == "OCR" && $themime == "application/octet-stream") {
                    $themime = "text/plain";
                  }
                  // Fedora labels are max 255 chars 
                  $dslabel = substr($filename,0,255);
                  //remove any trailing spaces
                  $dslabel = trim($dslabel);
                  $dslabel = !empty($dslabel) ? $dslabel : "$dsid datastream";
                  
                  $datastreams[$dsid] = array(
                    'dsid' => $dsid,
                    'label' => $dslabel,
                    'mimetype' => $themime,
                    'datastream_file' => $fileurl,
                    'filename' => $path_info['basename'],
                    'control_group' => 'M',
                  );
                }
                $z->close();
              }
              else {
                $errors[] = t('Provided ZIP file %zipfile can not be opened or is invalid. Code %errorcode.',
                array(
                  '%zipfile' => $this->preprocessorParameters['zipfile'],
                  '%errorcode' => (int)$opened,
                ));
                error_log('can not open ' . $this->preprocessorParameters['zipfile'] . ' with code:'. (int)$opened);
              }
            }

            // This will handle all cases, except ZIP.

            else {
              $fileurl = islandora_multi_importer_remote_file_get(trim($this->objectInfo['data'][$method[1]]));
              if ($method[0] == 'parent_object' && $fileurl) {
                $offset = trim($this->objectInfo['data'][$this->preprocessorParameters['object_maping']['sequencemap_row']['sequencemap']]);
                error_log($fileurl);
                error_log($device);
                error_log($offset);
                $fileurl = islandora_paged_content_extract_tiff_from_pdf($fileurl, $offset, $device, $resolution);
                // Get path info agaain for the returned mimetype
                $path_info = pathinfo(str_replace(" ", "\x20", trim($fileurl)));
                $themime = "image/tiff";
              }

              $themime = "application/octet-stream";
              if ($fileurl) {

                $themime = property_exists($fileurl, 'filemime') ? $fileurl->filemime : $this->getMimetype(drupal_basename($fileurl));
                //@TODO Check if anyone is dealing with a different filename (via a hook impl) 
                // than what ingesting user intended to set via the spreadsheet?
                if ($dsid == "HOCR" && $themime == "application/octet-stream") {
                  $themime = "text/html";
                }
                if ($dsid == "OCR" && $themime == "application/octet-stream") {
                  $themime = "text/plain";
                }
                $dslabel = substr($filename,0,255);
                //remove any trailing spaces
                $dslabel = trim($dslabel);
                $dslabel = !empty($dslabel) ? $dslabel : "$dsid datastream";

                $datastreams[$dsid] = array(
                  'dsid' => $dsid,
                  'label' => $dslabel,
                  'mimetype' => $themime,
                  'datastream_file' => $fileurl,
                  'filename' => drupal_basename($fileurl),
                  'control_group' => 'M',
                  // 'file' => $file,
                );
              }

              else { 

                // Not found...report the error.
                
                $errors[] = t('We tried, but For Object %id, %dsid datastream: %filename was no where to be found!',
                array(
                  '%filename' => trim($this->objectInfo['data'][$method[1]]),
                  '%dsid' => $dsid,
                  '%id' => $this->id,
                ));
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
   * Acquire base datastreams structure for update.
   */
  protected function getDatastreamsforUpdate(&$errors = NULL, &$files = NULL, AbstractObject $existing_object) {
    module_load_include('inc', 'islandora', 'includes/utilities');
    module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
    // Calling ::getDatastreams because we are tired of coding twice.
    $datastreams = $this->getDatastreams($errors, $files);
    if (isset($this['RELS-EXT']) && !empty($this['RELS-EXT'])) {
      // Does our existing object already have a RELS-EXT? 
      // I mean, maybe it is not there? ha.
      if (isset($existing_object['RELS-EXT'])) {
        $control = array(
          'control_group' => $existing_object['RELS-EXT']->controlGroup,
          'mimetype' => $existing_object['RELS-EXT']->mimeType,
          'label' => $existing_object['RELS-EXT']->label,
        );
      } 
      else {
        $control = array(
          'control_group' => $this['RELS-EXT']->controlGroup,
          'mimetype' => $this['RELS-EXT']->mimeType,
          'label' => $this['RELS-EXT']->label,
        );
      }
      $newfileNameExt = "RELSEXT_update_". urlencode($this->objectInfo['pid']) . '_' . time() . '.xml';
      // $tmp_directory = islandora_multi_importer_temp_directory();
      $newfileExt = file_save_data($this['RELS-EXT']->content, "public://" . $newfileNameExt , FILE_EXISTS_RENAME);
      $files[] = $newfileExt;
      $datastreams['RELS-EXT'] = array(
        'dsid' => 'RELS-EXT',
        'label' =>  $control['label'],
        'mimetype' => $control['mimetype'],
        'datastream_file' => drupal_realpath($newfileExt->uri),
        'filename' => $newfileNameExt ,
        'control_group' => $control['control_group'],
      );
    }
    
    if (isset($this['RELS-INT']) && !empty($this['RELS-INT'])) {
      // Does out existing object already have a RELS-INT? 
      // I mean, maybe it is not there? ha x2.
      if (isset($existing_object['RELS-INT'])) {
        $control = array(
          'control_group' => $existing_object['RELS-INT']->controlGroup,
          'mimetype' => $existing_object['RELS-INT']->mimeType,
          'label' => $existing_object['RELS-INT']->label,
        );
      } 
      else {
        $control = array(
          'control_group' => $this['RELS-INT']->controlGroup,
          'mimetype' => $this['RELS-INT']->mimeType,
          'label' => $this['RELS-INT']->label,
        ); 
      }
   
      $newfileNameInt = "RELSINT_update_". urlencode($this->objectInfo['pid']) . '_' . time() . '.xml';
      $newfileInt = file_save_data($this['RELS-INT']->content, "public://" . $newfileNameInt, FILE_EXISTS_RENAME);
      
      $files[] = $newfileInt;
      
      $datastreams['RELS-INT'] = array(
        'dsid' => 'RELS-INT',
        'label' =>  $control['label'],
        'mimetype' => $control['mimetype'],
        'datastream_file' => drupal_realpath($newfileInt->uri),
        'filename' => $newfileNameInt,
        'control_group' => $control['control_group'],
      );
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
   * Copies relevant RELS-EXT from to be updated object.
   *
   * @param AbstractObject $sourceObject
   *   An Object with properties we want to copy over.
   */
  protected function copyOtherRels(AbstractObject $sourceObject) {
    $rels = $sourceObject->relationships->get();
    foreach ($rels as $relationship) {
      if (($relationship['predicate']['namespace'] != "info:fedora/fedora-system:def/model#") && 
      ($relationship['predicate']['namespace']!=FEDORA_RELS_EXT_URI)) {
        $reltocheck = array();
        // Only avoid exactly the same ones
         $reltocheck = $this->relationships->get($relationship['predicate']['namespace'],$relationship['predicate']['value'],$relationship['object']['value'], $relationship['object']['literal']);
        if (empty($reltocheck)) {
          $this->relationships->add($relationship['predicate']['namespace'],$relationship['predicate']['value'], $relationship['object']['value'], $relationship['object']['literal']);
        }
      }
    }
  }
  
  /**
   * Copies relevant RELS-EXT from to be updated object.
   *
   * @param AbstractObject $sourceObject
   *   An Object with properties we want to copy over.
   */
  protected function copyOtherRelsUnique(AbstractObject $sourceObject) {
    $rels = $sourceObject->relationships->get();
    foreach ($rels as $relationship) {
      if (($relationship['predicate']['namespace'] != "info:fedora/fedora-system:def/model#") && 
      ($relationship['predicate']['namespace']!=FEDORA_RELS_EXT_URI)) {
        $reltocheck = array();
         // Only ones with the same predicates
         $reltocheck = $this->relationships->get($relationship['predicate']['namespace'],$relationship['predicate']['value']);
        if (empty($reltocheck) || ($relationship['predicate']['value'] == 'ManageableByRole') || ($relationship['predicate']['value'] == 'ManageableByUser')) {
          $this->relationships->add($relationship['predicate']['namespace'],$relationship['predicate']['value'], $relationship['object']['value'], $relationship['object']['literal']);
        }
      }
    }
  }

  /**
   * Copies fedora RELS-EXT from to be updated object.
   *
   * @param AbstractObject $sourceObject
   *   An Object with properties we want to copy over.
  */
  protected function copyFedoraRels(AbstractObject $sourceObject) {
   
    $rels = $sourceObject->relationships->get(FEDORA_RELS_EXT_URI);
    foreach ($rels as $relationship) {
      $reltocheck = array();
      // FEDORA_RELS_EXT_URI.
      $reltocheck = $this->relationships->get($relationship['predicate']['namespace'], $relationship['predicate']['value'], $relationship['object']['value']);
      
      // Only copy if not present in batchObject and value of existing is different from new parent
      if (empty($reltocheck) && ($relationship['object']['value']!= $this->objectInfo['parent'])) {
          $this->relationships->add($relationship['predicate']['namespace'],$relationship['predicate']['value'], $relationship['object']['value'], $relationship['object']['literal']);
      }
    }
  }
  
  /**
   * Copies fedora RELS-EXT from to be updated object.
   *
   * @param AbstractObject $sourceObject
   *   An Object with properties we want to copy over.
  */
  protected function copyFedoraRelsUnique(AbstractObject $sourceObject) {
   
    $rels = $sourceObject->relationships->get(FEDORA_RELS_EXT_URI);
    foreach ($rels as $relationship) {
      $reltocheck = array();
      // FEDORA_RELS_EXT_URI. Check if the same predicate is already here
      $reltocheck = $this->relationships->get($relationship['predicate']['namespace'], $relationship['predicate']['value']);
      
      // Only copy if not present in batchObject and value of existing is different from new parent
      if (empty($reltocheck)) {
          $this->relationships->add($relationship['predicate']['namespace'],$relationship['predicate']['value'], $relationship['object']['value'], $relationship['object']['literal']);
      }
    }
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
   * Fetch a valid parent RELS-EXT relationship for cmodel pair.
   */
  protected function setValidRelationshipsforUpdate(AbstractObject $existingobject) {
    // we need to figure out what diffs
    // so i will for now just call the same one, but
    // keep the specific method/wrapper for updates.
    $this->setValidRelationships();
  }


  /**
   * Add collection and content model relationships.
   */
  public function addRelationships() {
    if ($this->preprocessorParameters['action'] == 'update') {
      $this->addRelationshipsforUpdate(); 
    }
    else {
      $this->addRelationshipsforIngest(); 
    }
  }
  /**
   * Add collection and content model relationships.
   *  Only valid when ingesting new ones.
   */
  protected function addRelationshipsforIngest() {
    
    $this->setValidRelationships();
    $this->addContentModelRelationships();
    $this->inheritXacmlPolicies(); 
  }

  /**
   * Add collection and content model relationships.
   *  Only valid when ingesting new ones.
   */
  protected function addRelationshipsforUpdate() {
    $existing_object = islandora_object_load($this->id);
    $relsext_mode = isset($this->preprocessorParameters['object_maping']['relsext_row']['relsext_mode']) ? $this->preprocessorParameters['object_maping']['relsext_row']['relsext_mode'] : 'NEW';
    if ($existing_object) {
     
      // Copy existing member relationships over;
      if ($relsext_mode == 'NEW') {
          $this->addContentModelRelationships();
      // Sets new member relationships over
        $this->setValidRelationshipsforUpdate($existing_object);
      // sets scratch inheritance (new ones only)
        $this->inheritXacmlPoliciesForUpdate($existing_object); 
      }
      if  ($relsext_mode == 'MERGE') {
        $this->addContentModelRelationships();
        $this->setValidRelationshipsforUpdate($existing_object);
      // sets scratch inheritance (new ones only)
        $this->inheritXacmlPoliciesForUpdate($existing_object); 
        $this->copyFedoraRelsUnique($existing_object);
        $this->copyOtherRelsUnique($existing_object);
      }
      if  ($relsext_mode == 'JOIN') {
        $this->addContentModelRelationships();
        $this->setValidRelationshipsforUpdate($existing_object);
      // sets scratch inheritance (new ones only)
        $this->inheritXacmlPoliciesForUpdate($existing_object); 
        $this->copyFedoraRels($existing_object);
        $this->copyOtherRels($existing_object);
      }
      
      if ($relsext_mode == 'OLD') {
        // We don´t want any RELS DS to be processed.
          unset($this['RELS-EXT']);
          unset($this['RELS-INT']);
      }
    }
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
   * Add inheritXacmlFrom relationship.
   */
  protected function inheritXacmlPoliciesForUpdate(AbstractObject $existing_object) {
    if (module_exists('islandora_xacml_editor')) {
      $collection = $this->objectInfo['parent'];
      $parent = islandora_object_load($collection);
      $existing_object = islandora_object_load($this->id);
      if (($parent) && ($existing_object)) {
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
