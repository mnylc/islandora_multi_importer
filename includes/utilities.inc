<?php

/**
 * @file
 * includes/utilities.inc
 * 
 * Contains Parsing/Processing utilities 
 * @author Diego Pino Navarro
 */

@include_once 'sites/all/libraries/phpexcel/PHPExcel.php';

/**
 * Read Tabulated data into array.
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
    drupal_set_message(t('Could not parse file with error @error', array('@error'=>$e->getMessage())));
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
    foreach ($worksheet->getRowIterator() as $row) {
        $rowindex = $row->getRowIndex(); 
        if (($rowindex > 1) && ($rowindex > ($offset)) && (($rowindex <=($offset + $numrows +1)) || $numrows == -1 )) {
          $rowdata = array();
          // gets one row data
          $row =  $worksheet->rangeToArray("A{$rowindex}:".$highestColumn.$rowindex, NULL, TRUE, TRUE,FALSE);//Devuelve los titulos de cada columna
          $flat = trim(implode('', $row[0]));
          //check for empty row...if found stop there.
          if (strlen($flat)==0) {
            $maxRow = $rowindex;
            // @TODO check if this is not being overriden at line 64
            break;
          }
          $table[$rowindex] = $row[0];
          }
      $maxRow = $rowindex;
      }
  }
  $tabdata = array('headers' =>  $rowHeaders_utf8, 'data' => $table, 'totalrows'=> $maxRow);
  $objPHPExcel->disconnectWorksheets();
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
    if (!in_array($parsed_url['scheme'], $remote_schemes)) {
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

function islandora_multi_importer_twig_save($twig_name, $twigTemplate) {
   module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
   $twig_template_create_db = TwigTemplateDatabase::Create($twig_name, $twigTemplate);
   return $twig_template_create_db;
}

