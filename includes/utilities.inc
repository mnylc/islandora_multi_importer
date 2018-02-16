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
    } catch( Google_Service_Exception $e )  {
        drupal_set_message(t('Google API Error: @e',array('@e' => $e->getMessage())), 'error');
        return $tabdata;
    }
    $table = array();
    $headers = array();
    $maxRow = 0;
    $highestRow =  count($sp_data);
    if (($highestRow)>1) {
      // Returns Row Headers.
      $rowHeaders = $sp_data[0];
      $rowHeaders_utf8=array_map('stripslashes',$rowHeaders);
      $rowHeaders_utf8=array_map('utf8_encode',$rowHeaders_utf8);
      $rowHeaders_utf8=array_map('strtolower',$rowHeaders_utf8);
      $rowHeaders_utf8=array_map('trim',$rowHeaders_utf8);
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
            $table[$rowindex] = $row;
            }
        $maxRow = $rowindex;
        }
    }
  $tabdata = array('headers' => $sp_data[0], 'data' => $table, 'totalrows'=> $maxRow);
  
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

function islandora_multi_importer_remote_file_get($url) {
    $parsed_url = parse_url($url);
    $remote_schemes = array('http', 'https', 'ftp', 'ftps', 'smb', 'nfs');
    if (!isset($parsed_url['scheme']) || (isset($parsed_url['scheme']) &&!in_array($parsed_url['scheme'], $remote_schemes))) {
      // if local file, return the path.
      return drupal_realpath($url);
    }

    // Check if the temp directory exists if there is no other file named equally 
    // Create if needed.
    //$tmp_directory = islandora_multi_importer_temp_directory();
    $local =  "public://" . drupal_basename($parsed_url['path']);

    if (!file_exists($local)) {
      return system_retrieve_file($url, $local, FALSE, FILE_EXISTS_REPLACE);
    }
    else {
      return $local;
    }
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
    $path = 'public:///islandora-multi-importer/csv/';
    $filename = $user->uid . '-' . uniqid() . '.csv';
    // Ensure the directory
    if( !file_prepare_directory($path, FILE_CREATE_DIRECTORY) ){
      drupal_set_message( t('Unable to create directory for CSV file. Verify permissions please'), 'error' );
      return;
    }
    // Ensure the file
    $file = file_save_data('', $path . $filename);
    if( !$file ){
      drupal_set_message( t('Unable to create CSV file . Verify permissions please.'), 'error' );
      return;
    }
    $fh = fopen($file->uri, 'w');
    if( !$fh ){
      drupal_set_message( t('Error reading back the just written file!.'), 'error' );
      return;
    }
   
      fputcsv($fh, $data['headers']);
    
    foreach ($data['data'] as $row) {
      fputcsv($fh, $row);
    }
    fclose($fh);
    // Notify the filesystem of the size change
    $file->filesize = filesize($file->uri);
    $file->status = FILE_STATUS_PERMANENT;
    file_save($file);
      
    // Tell the user where we stuck it
   
    drupal_set_message( t('CSV file saved and available at. <a href="!url">!filename</a>.', array(
      '!url' => file_create_url($file->uri),
      '!filename' => $filename,
    )));
    
    return $file->fid;
}
