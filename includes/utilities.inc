<?php

/**
 * @file
 * includes/utilities.inc
 *
 * Contains Parsing/Processing utilities
 * @author Diego Pino Navarro
 */


/**
 * Wrapper/Chooser for Data source
 *
 * @param array $form_state
 *   $form_state with valid src. options.
 * @param int $page
 *   which page, defaults to 0.
 * @param int $per_page
*   number of records per page, -1 means all.
 * @return array
 *   array of associative arrays containing header and data as header => value pairs
 */
function islandora_multi_importer_datasource_loader(array $form_state, $page = 0, $per_page = 20) {
  //dpm($form_state);
  if ($form_state['storage']['values']['step0']['data_source'] == 'google' && !empty($form_state['storage']['values']['step2']['google_api']['spreadsheet_id'])) {
    $spreadsheetId = trim($form_state['storage']['values']['step2']['google_api']['spreadsheet_id']);
    // Parse the ID from the URL if a full URL was provided.
    // @author of following chunk is Mark Mcfate @McFateM!
    if ($parsed = parse_url($spreadsheetId)) {
      if (isset($parsed['scheme'])) {
      $parts = explode('/', $parsed['path']);
      $spreadsheetId = $parts[3];
     }
   }
    $range = trim($form_state['storage']['values']['step2']['google_api']['spreadsheet_range']);
    //dpm($range);
    $file_data = islandora_multi_importer_read_googledata($spreadsheetId, $range, $per_page, $page * $per_page);

  }
  else {
    $file = file_load($form_state['storage']['values']['step1']['file']);
    $file_path = drupal_realpath($file->uri);
    $file_data = islandora_multi_importer_read_filedata($file_path, $per_page, $page * $per_page);
  }
  return $file_data;
}

/**
 * Read Tabulated data from file into array.
 *
 * @param url $file_path
 *   Path to file
 * @param int $numrows
 *   Number of rows to return, -1 magic number means all
 * @param int $offset
 *   Offset for rows to return
 *
 * @return array
 *   array of associative arrays containing header and data as header => value pairs
 */
function islandora_multi_importer_read_filedata($file_path, $numrows = 20, $offset = 0) {

  $tabdata = array('headers' => array(), 'data' => array(), 'totalrows' => 0);
  try {
    $inputFileType = PHPExcel_IOFactory::identify($file_path);

    $objReader = PHPExcel_IOFactory::createReader($inputFileType);

    $objReader->setReadDataOnly(true);
    $objPHPExcel = $objReader->load($file_path);
  } catch(Exception $e) {
      drupal_set_message(t('Could not parse file with error: @error', array('@error'=>$e->getMessage())));
    return $tabdata;
  }
  $table = array();
  $headers = array();
  $maxRow = 0;
  $worksheet = $objPHPExcel->getActiveSheet();
  $highestRow =  $worksheet->getHighestRow();
  $highestColumn =  $worksheet->getHighestColumn();
  if (($highestRow)>1) {
    // Returns Row Headers.
    $rowHeaders =  $worksheet->rangeToArray('A1:'.$highestColumn.'1', NULL, TRUE, TRUE,FALSE);
    $rowHeaders_utf8=array_map('stripslashes',$rowHeaders[0]);
    $rowHeaders_utf8=array_map('utf8_encode',$rowHeaders_utf8);
    $rowHeaders_utf8=array_map('strtolower',$rowHeaders_utf8);
    $rowHeaders_utf8=array_map('trim',$rowHeaders_utf8);
    foreach ($worksheet->getRowIterator() as $row) {
        $rowindex = $row->getRowIndex();
        if (($rowindex > 1) && ($rowindex > ($offset)) && (($rowindex <=($offset + $numrows +1)) || $numrows == -1 )) {
          $rowdata = array();
          // gets one row data
          $datarow =  $worksheet->rangeToArray("A{$rowindex}:".$highestColumn.$rowindex, NULL, TRUE, TRUE,FALSE);//Devuelve los titulos de cada columna
          $flat = trim(implode('', $datarow[0]));
          //check for empty row...if found stop there.
          if (strlen($flat) == 0) {
            $maxRow = $rowindex;
            // @TODO check if this is not being overriden at line 64
            break;
          }
          $table[$rowindex] = $datarow[0];
          }
      $maxRow = $rowindex;
      }
  }
  $tabdata = array('headers' =>  $rowHeaders_utf8, 'data' => $table, 'totalrows'=> $maxRow);
  $objPHPExcel->disconnectWorksheets();
  return $tabdata;
  }

  /**
   * Read Tabulated data coming from Google into array.
   *
   * @param url $file_path
   *   Path to file
   * @param string $range,
   *   Google API`s expected Range value in the form of 'Sheet1!A1:B10'.
   * @param int $numrows
   *   Number of rows to return, -1 magic number means all
   * @param int $offset
   *   Offset for rows to return
   * @return array
   *   array of associative arrays containing header and data as header => value pairs
   */
  function islandora_multi_importer_read_googledata($spreadsheetId, $range = 'Sheet1!A1:B10', $numrows = 20, $offset = 0) {
    $tabdata = array('headers' => array(), 'data' => array(), 'totalrows' => 0);
    $sp_data = array();
    $rowdata = array();


    // Establish a connection first
    $client = islandora_multi_importer_googleclient_connect();
    try {
         $service = new Google_Service_Sheets($client);

         $response = $service->spreadsheets_values->get($spreadsheetId, $range);
         $sp_data = $response->getValues();
         // Empty value? just return
         if (($sp_data == NULL) OR empty($sp_data)) {
           drupal_set_message(t('Nothing to read, check your Data source content'), 'error');
           return $tabdata;

         }
    } catch( Google_Service_Exception $e )  {
        drupal_set_message(t('Google API Error: @e',array('@e' => $e->getMessage())), 'error');
        return $tabdata;
    }
    $table = array();
    $headers = array();
    $maxRow = 0;
    $highestRow =  count($sp_data);

    $rowHeaders = $sp_data[0];
    $rowHeaders_utf8=array_map('stripslashes',$rowHeaders);
    $rowHeaders_utf8=array_map('utf8_encode',$rowHeaders_utf8);
    $rowHeaders_utf8=array_map('strtolower',$rowHeaders_utf8);
    $rowHeaders_utf8=array_map('trim',$rowHeaders_utf8);

    $headercount = count($rowHeaders);

    if (($highestRow) >=1 ) {
      // Returns Row Headers.

      $maxRow = 1; // at least until here.
      foreach ($sp_data as $rowindex => $row) {

        // Google Spreadsheets start with Index 0. But PHPEXCEL, parent`s
        // function does with 1.
        // To keep both function responses in sync using the same params, i will compensate offsets here:

          if (($rowindex >= 1) && ($rowindex > ($offset-1)) && (($rowindex <=($offset + $numrows)) || $numrows == -1 )) {
            $rowdata = array();
            // gets one row data

            $flat = trim(implode('', $row));
            //check for empty row...if found stop there.
            if (strlen($flat) == 0) {
              $maxRow = $rowindex;
              break;
            }
            $row = islandora_multi_importer_array_equallyseize($headercount, $row);
            $table[$rowindex] = $row;
            }
        $maxRow = $rowindex;
        }
    }
  $tabdata = array('headers' => $rowHeaders_utf8, 'data' => $table, 'totalrows'=> $maxRow);

  return $tabdata;
}

function islandora_multi_importer_xml_highlight($s){
      $s = htmlspecialchars($s);
      $s = preg_replace("#&lt;([/]*?)(.*)([\s]*?)&gt;#sU",
          "<font color=\"#0000FF\">&lt;\\1\\2\\3&gt;</font>",$s);
      $s = preg_replace("#&lt;([\?])(.*)([\?])&gt;#sU",
          "<font color=\"#800000\">&lt;\\1\\2\\3&gt;</font>",$s);
      $s = preg_replace("#&lt;([^\s\?/=])(.*)([\[\s/]|&gt;)#iU",
          "&lt;<font color=\"#808000\">\\1\\2</font>\\3",$s);
      $s = preg_replace("#&lt;([/])([^\s]*?)([\s\]]*?)&gt;#iU",
          "&lt;\\1<font color=\"#808000\">\\2</font>\\3&gt;",$s);
      $s = preg_replace("#([^\s]*?)\=(&quot;|')(.*)(&quot;|')#isU",
          "<font color=\"#800080\">\\1</font>=<font color=\"#FF00FF\">\\2\\3\\4</font>",$s);
      $s = preg_replace("#&lt;(.*)(\[)(.*)(\])&gt;#isU",
          "&lt;\\1<font color=\"#800080\">\\2\\3\\4</font>&gt;",$s);
      return nl2br($s);
  }
  /**
   * Checks if an URI from spreadsheet is remote or local and returns a file
   *
   * @param string $url
   *   The URL of the file to grab.
   * @return mixed
   *   One of these possibilities:
   *   - If remote and exists a Drupal file object
   *   - If not remote and exists a stripped file object
   *   - If does not exist boolean FALSE
   */
function islandora_multi_importer_remote_file_get($url) {
    $parsed_url = parse_url($url);
    $remote_schemes = array('http', 'https', 'feed');
    if (!isset($parsed_url['scheme']) || (isset($parsed_url['scheme']) && !in_array($parsed_url['scheme'], $remote_schemes))) {
      // If local file, engage any hook_islandora_multi_importer_remote_file_get and return the real path.
      $path = array();
      $path = module_invoke_all('islandora_multi_importer_remote_file_get', $url);
      // get only the first path.
      if (!empty($path)) {
        if ($path[0]) {
          return $path[0];
        }
      }

      // if local file, try the path.
      $localfile = drupal_realpath($url);
      if (!file_exists($localfile)) {
        return FALSE;
        }
      return $localfile;
    }

    $localfile = _imi_build_file_storage_uri(drupal_basename($parsed_url['path']), 'remote-files');
    if (!file_exists($localfile)) {
      // Actual remote heavy lifting only if not present.
      $localfile = islandora_multi_importer_retrieve_remote_file($url, $localfile, FILE_EXISTS_REPLACE);
      return $localfile;
    }
    else {
      return $localfile;
    }
    return FALSE;
  }

/**
 * Attempts to get a file using drupal_http_request and to store it locally.
 *
 * @param string $url
 *   The URL of the file to grab.
 * @param string $destination
 *   Stream wrapper URI specifying where the file should be placed. If a
 *   directory path is provided, the file is saved into that directory under
 *   its original name. If the path contains a filename as well, that one will
 *   be used instead.
 *   If this value is omitted, the site's default files scheme will be used,
 *   usually "public://".
 * @param int $replace
 *   Replace behavior when the destination file already exists:
 *   - FILE_EXISTS_REPLACE: Replace the existing file.
 *   - FILE_EXISTS_RENAME: Append _{incrementing number} until the filename is
 *     unique.
 *   - FILE_EXISTS_ERROR: Do nothing and return FALSE.
 *
 * @return mixed
 *   One of these possibilities:
 *   - If it succeeds an managed file object
 *   - If it fails, FALSE.
 */
function islandora_multi_importer_retrieve_remote_file($url, $destination = NULL, $replace = FILE_EXISTS_RENAME) {
    module_load_include('inc', 'islandora', 'includes/mimemtype.utils');
    // pre set a failure
    $localfile = FALSE;
    $parsed_url = parse_url($url);
    $mime = 'application/octet-stream';
    if (!isset($destination)) {
      $path = _imi_build_file_storage_uri(drupal_basename($parsed_url['path']), 'remote-files');
    }
    else {
      if (is_dir(drupal_realpath($destination))) {

        // Prevent URIs with triple slashes when glueing parts together.
        $path = str_replace('///', '//', "{$destination}/") . drupal_basename($parsed_url['path']);
      }
      else {
        $path = $destination;
      }
    }
    if(!$path) {
      return FALSE;
    }
    $result = drupal_http_request($url);
    if ($result->code != 200) {
      drupal_set_message(t('HTTP error @errorcode occurred when trying to fetch @remote.', array(
        '@errorcode' => $result->code,
        '@remote' => $url,
      )), 'error');
      return FALSE;
    }

    // It would be more optimal to run this after saving
    // but i really need the mime in case no extension is present
    $mimefromextension = file_get_mimetype($path, NULL);

    if (($mimefromextension == "application/octet-stream") &&
      isset($result->headers['Content-Type'])) {
      $mimetype = $result->headers['Content-Type'];
      $extension = islandora_get_extension_for_mimetype($mimetype);
      $info = pathinfo($path);
      if (($extension != "bin") && ($info['extension']!= $extension)) {
        $path = $path.".".$extension;
      }
    }
    // File is being made managed and permanent here, will be marked as
    // temporary once it is processed AND/OR associated with a SET
    $localfile = file_save_data($result->data, $path, $replace);
    if (!$localfile) {
      drupal_set_message(t('@remote could not be saved to @path.', array(
        '@remote' => $url,
        '@path' => $path,
      )), 'error');
    }

    return $localfile;
  }



function islandora_multi_importer_temp_directory($create = TRUE) {
    $directory = &drupal_static(__FUNCTION__, '');
    if (empty($directory)) {
      $directory = 'temporary://islandora-multi-importer';
      if ($create && !file_exists($directory)) {
        mkdir($directory);
      }
    }
    return $directory;
  }

function islandora_multi_importer_twig_process(array $twig_input = array()) {
  if (count($twig_input) == 0) {
    return;
  }
  $loader = new Twig_Loader_Array(array(
    $twig_input['name'] => $twig_input['template'],
  ));

  $twig = new \Twig_Environment($loader, array(
    'cache' => drupal_realpath('private://'),
  ));

  //We won't validate here. We are here because our form did that
  $output = $twig->render($twig_input['name'], $twig_input['data']);
  //@todo catch anyway any twig error to avoid the worker to fail bad.

  return $output;
}

function islandora_multi_importer_twig_list() {
  $twig_templates_from_db = &drupal_static(__FUNCTION__);
  if (!isset($twig_templates_from_db)) {
    module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
    $twig_templates_from_db = TwigTemplateDatabase::GetNames();
  }
return $twig_templates_from_db;
}

function islandora_multi_importer_twig_fetch($twig_name) {
  $twig_template_from_db = &drupal_static(__FUNCTION__);
  if (!isset($twig_template_from_db)) {
    module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
    $twig_template_from_db = TwigTemplateDatabase::Get($twig_name);
  }
return $twig_template_from_db;
}
/**
 * Fetches a twig template by it's id.
 *
 * @param int $twig id
 *   Surprinsingly the ID of the twig template in DB.
 *
 * @return array
 * associative array with following structure
 * 'name' => the name of the template
 * 'id' => the unique integer id
 * 'twig' => the template itself
 * 'updated' => when this one was updated
 */
function islandora_multi_importer_twig_fetchbyid($twig_id) {
  $twig_template_info_from_db = &drupal_static(__FUNCTION__);
  if (!isset($twig_template_from_db)) {
    module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
    $twig_template_info_from_db = TwigTemplateDatabase::GetbyId($twig_id);
  }

return $twig_template_info_from_db;
}
/**
 * Saves a twig template by it's name.
 *
 * @param string $twig_name
 *   The future name of the twig template in DB.
 * @param string $twigTemplate
 *   Surprinsingly the Template itself.
 *
 * @return bool
 */
function islandora_multi_importer_twig_save($twig_name, $twigTemplate) {
   module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
   $twig_template_create_db = TwigTemplateDatabase::Create($twig_name, $twigTemplate);
   return $twig_template_create_db;
}

/**
 * Updates an existing twig template by ID.
 *
 * @param string $twig_name
 *   The future name of the twig template in DB.
 * @param string $twigTemplate
 *   Surprinsingly the Template itself.
 *
 * @return bool
 */
function islandora_multi_importer_twig_update($twig_id, $twig_name, $twigTemplate) {
   module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
   $twig_template_create_db = TwigTemplateDatabase::UpdateById($twig_id, $twig_name, $twigTemplate);
   return $twig_template_create_db;
}
/**
 * Deletes a twig template by it's id.
 *
 * @param int $twig id
 *   Surprinsingly the ID of the twig template in DB.
 *
 * @return bool
 */
function islandora_multi_importer_twig_deletebyid($twig_id) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
  $twig_template = TwigTemplateDatabase::DeletebyId($twig_id);
  return $twig_template;
}

/**
 * Creates an CSV from array and returns file.
 *
 * @param array $data
 *   Same as import form handles, to be dumped to CSV.
 *
 * @return file
 */
function islandora_multi_importer_csv_save(array $data) {
    global $user;
    $filename = $user->uid . '-' . uniqid() . '.csv';
    $path = _imi_build_file_storage_uri($filename, 'csv');
    // Ensure the directory
    if( !$path){
      drupal_set_message( t('Unable to create directory for CSV file. Verify permissions please'), 'error' );
      return;
    }
    // Ensure the file
    $file = file_save_data('', $path);
    if( !$file ){
      drupal_set_message( t('Unable to create CSV file . Verify permissions please.'), 'error' );
      return;
    }
    $fh = fopen($file->uri, 'w');
    if( !$fh ){
      drupal_set_message( t('Error reading back the just written file!.'), 'error' );
      return;
    }
      array_walk($data['headers'],'htmlspecialchars');
      fputcsv($fh, $data['headers']);

    foreach ($data['data'] as $row) {
      array_walk($row,'htmlspecialchars');
      fputcsv($fh, $row);
    }
    fclose($fh);
    // Notify the filesystem of the size change
    $file->filesize = filesize($file->uri);
    $file->status = ~FILE_STATUS_PERMANENT;
    file_save($file);

    // Tell the user where we stuck it

    drupal_set_message( t('CSV file saved and available at. <a href="!url">!filename</a>.', array(
      '!url' => file_create_url($file->uri),
      '!filename' => $filename,
    )));

    return $file->fid;
}

/**
 * Deal with different sized arrays for combining
 *
 * @param array $header
 *   a CSV header row
 * @param array $row
*   a CSV data row
 *
 * @return array
 * combined array
 */
function islandora_multi_importer_array_combine_special($header, $row) {
    $headercount = count($header);
    $rowcount = count($row);
    if ($headercount > $rowcount) {
        $more = $headercount - $rowcount;
        for($i = 0; $i < $more; $i++) {
            $row[] = "";
        }

    }
    else if ($headercount < $rowcount) {
      // more fields than headers
      // Header wins always
      $row = array_slice($row, 0, $headercount);
    }

    return array_combine($header, $row);
}


/**
 * Match different sized arrays.
 *
 * @param array $headercount
 *   an array lenght to check against.
 * @param array $row
*   a CSV data row
 *
 * @return array
 *  a resized to header size data row
 */
function islandora_multi_importer_array_equallyseize($headercount, $row) {

    $rowcount = count($row);
    if ($headercount > $rowcount) {
        $more = $headercount - $rowcount;
        for($i = 0; $i < $more; $i++) {
            $row[] = "";
        }

    }
    else if ($headercount < $rowcount) {
      // more fields than headers
      // Header wins always
      $row = array_slice($row, 0, $headercount);
    }

    return $row;
}

/**
 * Checks if a given value can be used as Fedora object DSID
 *
 * @param string $dsid
 *   the dsid candidate
 * @return bool
 *  True if it can be used as DSID
 */
function islandora_multi_importer_isValidXmlName($dsid) {
  $dsid = trim($dsid);
  // see https://wiki.duraspace.org/display/FEDORA38/Fedora+Identifiers
  if ((strlen($dsid)>64) || (strlen($dsid)==0)) {
    return FALSE;
  }

  try {
    new DOMElement($dsid);
      return TRUE;
  } catch(DOMException $e) {
      return FALSE;
  }
}

/**
 * Constructs a URI to IMI's file storage location for a file.
 * Create the directory if it does not exist.
 *
 * @param string $filename
 *   The name of the file to be stored.
 *   Required.
 * @param string $imi_temp_subdir
 *   A sub-path.
 *   Optional.
 * @param string $imi_temp_dir
 *   The islandora-multi-importer directory location relative to the uri scheme.
 *   Optional. Defaults to 'islandora-multi-importer'.
 * @param string $scheme
 *   The name of the drupal stream wrapper scheme.
 *   Optional. Defaults to "private".
 *
 * @return string
 *   Returns a normalized stream wrapper uri to the file storage location.
 *   Returns FALSE if unable to create/verify the file storage directory.
 */
function _imi_build_file_storage_uri($filename, $imi_temp_subdir = NULL, $imi_temp_dir = NULL, $scheme = 'private' ) {

  $directory_uri = _imi_build_file_storage_directory($imi_temp_subdir, $imi_temp_dir, $scheme);
  if(empty($directory_uri)) {
    drupal_set_message(t('Error occurred while preparing storage for @filename in @directory. Check destination folder permissions', array(
      '@filename' => $filename,
      '@directory' => $directory_uri,
    )), 'error');
    return FALSE;
  }
  // Assemble and return the full file uri.
  return file_stream_wrapper_uri_normalize($directory_uri . DIRECTORY_SEPARATOR . $filename);
}

/**
 * Creates a temporary file storage directory
 *
 * @param string   $imi_temp_subdir
 *   A subdirectory path.
 *   Optional - None will be used if not provided.
 * @param string   $imi_temp_dir
 *   The main imi temporary files directory.
 *   Optional - Default will be used if none is provided.
 * @param string $scheme
 *   The Drupal filestream scheme (public://, private://, temporary://).
 *   Optional - private:// will be used if none is provided.
 *
 * @return bool/string
 *   The uri to the new directory if successful.
 *   FALSE if unable to create the directory.
 */
function _imi_build_file_storage_directory($imi_temp_subdir = NULL, $imi_temp_dir = NULL, $scheme = 'private') {

  $directory = array(!empty($imi_temp_dir) ? $imi_temp_dir : variable_get('islandora_multi_importer_temp_dir', 'islandora-multi-importer'));

  if(!in_array($scheme, stream_get_wrappers())) {
    drupal_set_message(t('Could not create IMI temporary directory in <em>@scheme://</em>, as that stream wrapper is not defined. Ensure that the <em>@scheme://</em> file directory is defined and is writable at <a href="/admin/config/media/file-system">Configuration > Media > File system</a>', array(
      '@scheme' => $scheme,
    )), 'error');
    return FALSE;
  }

  if(!empty($imi_temp_subdir)) {
    $directory[] = $imi_temp_subdir;
  }
  $directory_uri = file_stream_wrapper_uri_normalize($scheme . '://' . implode(DIRECTORY_SEPARATOR, $directory));

  $writable = file_prepare_directory($directory_uri, FILE_CREATE_DIRECTORY);
  if(!$writable) {
    drupal_set_message(t('Error occurred while preparing destination directory @directory. Check configuration for the @scheme:// directory location at <a href="/admin/config/media/file-system"> ', array(
      '@directory' => $directory_uri,
      '@scheme' => $scheme,
    )), 'error');
    return FALSE;
  }
  return $directory_uri;
}