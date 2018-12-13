<?php

/**
 * @file
 * Functions for generating/validating/submitting the multi import objects form.
 * @author Diego Pino Navarro
 */

/**
 * The main import form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The drupal form definition.
 */
function islandora_multi_importer_form(array $form, array &$form_state) {
  module_load_include('inc', 'islandora', 'includes/content_model');
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_multi_importer', 'includes/mapper');
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  module_load_include('inc', 'islandora_multi_importer', 'includes/googleapi.inc');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/import.form');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/utilities');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/googleapi');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/callbacks');
 
  // Just to be sure Ajax is loaded. if no element makes reference to it 
  // Drupal skip`s it, but then our custom gives warnings on those steps
  drupal_add_library('system', 'drupal.ajax');
  drupal_add_library('system', 'jquery.form');
  
  $step = empty($form_state['storage']['step']) ? 0 : $form_state['storage']['step'];
  
  $form_state['storage']['step'] = $step;
  $step_forms = array();
  switch ($step) {
    case 0:
      drupal_set_title(t("Islandora Multi importer - Data Source Selection Step"));
      $step_forms['step0'] = islandora_multi_importer_choosesource_form($form, $form_state);
      $step_forms['submit'] = array(
        '#type' => 'submit',
        '#name' => 'next',
        '#value' => t('Next'),
        '#weight' => 100,
      );
      break;
    case 1:
      drupal_set_title(t("Islandora Multi importer - Data Source: Upload File Step"));
      $step_forms['step1'] = islandora_multi_importer_upload_form($form, $form_state);
      $step_forms['#validate'] = array('islandora_multi_importer_form_fileisuploaded_validate');
      $step_forms['submit'] = array(
        '#type' => 'submit',
        '#name' => 'preprocess',
        '#value' => t('Preprocess'),
        '#weight' => 100,
        '#submit' => array('islandora_multi_importer_form_submit'),
      );
      $step_forms['back'] = array(
        '#type' => 'submit',
        '#name' => 'back',
        '#value' => t('Back to Data Source type selection'),
        '#weight' => 100,
        '#limit_validation_errors' => array(),
        '#submit' => array('islandora_multi_importer_form_submit'),
        );
      break;
    
    case 2:
      drupal_set_title(t("Islandora Multi importer - Data Source: Google Sheet info"));
      $step_forms['#tree'] = TRUE;
      $step_forms['step2'] = islandora_multi_importer_google_form($form, $form_state);
      $step_forms['#validate'] = array('islandora_multi_importer_form_googlecanread_validate');
      $step_forms['submit'] = array(
          '#type' => 'submit',
          '#name' => 'preprocess',
          '#value' => t('Preprocess'),
          '#weight' => 101,
          '#submit' => array('islandora_multi_importer_form_submit'),
        );
      
      $step_forms['back'] = array(
        '#type' => 'submit',
        '#name' => 'back',
        '#value' => t('Back to Data Source type selection'),
        '#weight' => 100,
        '#limit_validation_errors' => array(),
        '#submit' => array('islandora_multi_importer_form_submit'),
        );
      break;

      case 3:
      drupal_set_title(t("Islandora Multi importer - Import settings step"));
      // This keeps our file data around between rebuilds
      $file_data = isset($form_state['storage']['values']['general']['file_data']) ? $form_state['storage']['values']['general']['file_data'] : array();
      $step_forms['maintab']['#type'] = 'horizontal_tabs';

      $step_forms['#tree'] = TRUE;

      $step_forms['step3'] = (array) islandora_multi_importer_previs_form($form, $form_state, $file_data);

      $step_forms['step4'] = (array) islandora_multi_importer_twig_form($form, $form_state, $file_data);
      $step_forms['cmodelmap'] = (array) islandora_multi_importer_cmodelmapping_form($form, $form_state, $file_data);
      $step_forms['objectmap'] = (array) islandora_multi_importer_objectmapping_form($form, $form_state, $file_data);
      $step_forms['step3']['#id'] = drupal_html_id('main-data2');
      $step_forms['step3']['#group'] = 'maintab';
      $step_forms['step3']['#collapsible'] = FALSE;
      $step_forms['step3']['#access'] = TRUE;
      $step_forms['step3']['#type'] = 'fieldset';
      $step_forms['step3']['#title'] = t('Your data');

      $step_forms['step4']['#id'] = drupal_html_id('main-data3');
      $step_forms['step4']['#group'] = 'maintab';
      $step_forms['step4']['#collapsible'] = FALSE;
      $step_forms['step4']['#access'] = TRUE;
      $step_forms['step4']['#type'] = 'fieldset';
      $step_forms['step4']['#title'] = t('Templating');
      
      $step_forms['cmodelmap']['#id'] = drupal_html_id('main-data4');
      $step_forms['cmodelmap']['#group'] = 'maintab';
      $step_forms['cmodelmap']['#collapsible'] = FALSE;
      $step_forms['cmodelmap']['#access'] = TRUE;
      $step_forms['cmodelmap']['#type'] = 'fieldset';
      $step_forms['cmodelmap']['#title'] = t('CMODEL Mapping');

      $step_forms['objectmap']['#id'] = drupal_html_id('main-data5');
      $step_forms['objectmap']['#group'] = 'maintab';
      $step_forms['objectmap']['#collapsible'] = FALSE;
      $step_forms['objectmap']['#access'] = TRUE;
      $step_forms['objectmap']['#type'] = 'fieldset';
      $step_forms['objectmap']['#title'] = t('Object Properties Mapping');
      $step_forms['back'] = array(
        '#name' => 'ini',
        '#type' => 'submit',
        '#weight' => 100,
        '#value' => t('Back to data source selection'),
      );
      if ($form_state['storage']['values']['step0']['data_source'] == 'google' && !empty($form_state['storage']['values']['step2']['google_api']['spreadsheet_id'])) {
      $step_forms['reread'] = array(
          '#type' => 'submit',
          '#name' => 'reread',
          '#value' => t('Re-read data from google'),
          '#weight' => 101,
          '#submit' => array('islandora_multi_importer_form_submit'),
        );
      }
      $step_forms['ingest'] = array(
        '#type' => 'submit',
        '#weight' => 102,
        '#value' => t('Ingest'),
        
      );

      // We should only show this one if required ingest info is set.
      if (class_exists('ZMQContext')) {
        $step_forms['ingest_option'] = array(
          '#type' => 'checkbox',
          '#value' => FALSE,
          '#title' => t('Use zeroMQ processing'),
          '#description' => t('This sends processing to multiple background workers'),
        );
      }
      break;

    case 4:
      drupal_set_title(t("Islandora Multi importer - Binary Source ZIP upload step"));
      $step_forms['last'] = !variable_get('islandora_multi_importer_plupload', FALSE) ? islandora_multi_importer_zip_upload_form($form, $form_state) : islandora_multi_importer_zip_plupload_form($form, $form_state);
      $step_forms['ingest'] = array(
        '#type' => 'submit',
        '#value' => t('Ingest'),
      );

      // We should only show this one if required ingest info is set.
      if (class_exists('ZMQContext')) {
        $step_forms['ingest_option'] = array(
          '#type' => 'checkbox',
          '#value' => FALSE,
          '#title' => t('Use zeroMQ processing'),
          '#description' => t('This sends processing to multiple background workers'),
        );
      }
      break;
  }
  if (($step == 3) || ($step ==4)) {
    $step_forms['action'] = array(
      '#type' => 'select',
      '#title' => t('What type of Batch Action to execute?'),
      '#options' => array('ingest'=>'ingest new objects','update'=>'update existing objects'),
      '#default_value' => 'ingest' ,
      '#ajax' => array(
        'callback' => 'islandora_multi_importer_changeaction_ajax',
        'wrapper' => 'islandora-multi-top',
      ),
    );
  }
  $form = $step_forms;
  // add JS
  $form['#attached']['js'][] = drupal_get_path('module', 'islandora_multi_importer') . '/js/utils.js';
 
  $form['#prefix'] = '<div id="islandora-multi-top">';
  $form['#suffix'] = '</div>';
  $form['#tree'] = TRUE;
  return $form;
}


function islandora_multi_importer_form_fileisuploaded_validate($form, &$form_state) {

  // If the file passed validation:
  // Weird but once people push the upload button file_save_upload 
  // had run already on a different level. So need to skip if that
  // is the case.

  if ($form_state['clicked_button']['#name'] == 'back') {
    return;
  }

  if (!$form_state['values']['step1']['file']) {
  $file = file_save_upload('step1_file', array(
    'file_validate_extensions' => array('txt csv tsv xls xlsx')),
    FILE_EXISTS_REPLACE);

   if ($file) {
     // Move the file into the Drupal file system.
     if ($file = file_move($file, 'public://')) {
       // Save the file for use in the submit handler.
       $form_state['storage']['file'] = $file;
     }
     else {
       form_set_error('file', t("Failed to write the uploaded file to the site's file folder."));
     }
   }
   else {
     form_set_error('file', t('No file was uploaded.'));
   }
 }
}

function islandora_multi_importer_form_googlecanread_validate($form, &$form_state) {

  if ($form_state['clicked_button']['#name'] == 'back') {
    return;
  }

  
  $spreadsheetId = trim($form_state['values']['step2']['google_api']['spreadsheet_id']);
  $range = trim($form_state['values']['step2']['google_api']['spreadsheet_range']);
  if (!empty($spreadsheetId) && !empty($range)) {
  // Parse the ID from the URL if a full URL was provided.
  // @author of following chunk is Mark Mcfate @McFateM!
    if ($parsed = parse_url($spreadsheetId)) {
      if (isset($parsed['scheme'])) {
        $parts = explode('/', $parsed['path']);
        $spreadsheetId = $parts[3];
      }
    }
    $googledata = islandora_multi_importer_read_googledata($spreadsheetId, $range, -1, 0);
   
    if ($googledata['totalrows'] == 0 || empty($googledata['headers'])) {
      drupal_set_message(t('Seems like your Google Spreadsheet has no data, no headers or we are having trouble fetching it. Check your ID,  and try again URL or Range.'),'error');
      form_set_error('google_api', t('Wrong Google Sheet info provided'));
    }
  }
  else {
    form_set_error('google_api', t('All Google Sheet info fields are required')); 
  }
}




/**
 * Spreadsheet upload data form.
 */
function islandora_multi_importer_upload_form($form, &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $form_file = array(
    'file' => array(
      '#type' => 'managed_file',
      '#required' => true,
      '#title' => t('CSV,TSV,XLS or XLSX file containing the row(s) to import'),
      '#size' => 48,
      '#description' => t('Select a file to upload.<br/>Files must be less than <strong>@size MB.</strong>', array('@size' => $upload_size)),
      '#default_value' => isset($form_state['values']['file']) ? $form_state['values']['file'] : NULL,
      '#upload_location' => file_default_scheme() . '://',
      '#upload_validators' => array(
        'file_validate_extensions' => array('txt csv tsv xls xlsx'),
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );
  return $form_file;
}



/**
 * Google Sheet ID
 */
function islandora_multi_importer_google_form($form, &$form_state) {
  
  // I like this approach on multistep defaults.
  $stored_defaults = array('spreadsheet_id' => '', 'spreadsheet_range' => 'Sheet1!A1:B10');
  
  if (isset($form_state['storage']['values']['step'.$form_state['storage']['step']])) {
     $stored_defaults['spreadsheet_id'] = $form_state['storage']['values']['step'.$form_state['storage']['step']]['google_api']['spreadsheet_id'];
     $stored_defaults['spreadsheet_range'] = $form_state['storage']['values']['step'.$form_state['storage']['step']]['google_api']['spreadsheet_range'];
  }
  
  $form['google_api']= array(
    '#prefix' => '<div id="islandora-multi-googleapi">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    'spreadsheet_id' => array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#title' => t('ID of your Google Sheet'),
      '#description' => 'Example: https://docs.google.com/spreadsheets/d/aaBAccEEFfC_aBC-aBc0d1EF/edit, use aaBAccEEFfC_aBC-aBc0d1EF portion as the ID or full URL',
      '#default_value' => $stored_defaults['spreadsheet_id'],
    ),
    'spreadsheet_range' => array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#title' =>  t('Cell Range'),
      '#description' => t('Cell Range for your Google Sheet, in the form of SheetName!A1:B10'),
      '#default_value' => $stored_defaults['spreadsheet_range'],
    ),
   ); 
  return $form;
}

/**
 * ZIP Upload sub-form for islandora_multi_importer_upload_form.
 */
function islandora_multi_importer_zip_upload_form($form, &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $form = array(
    'file2' => array(
      '#type' => 'managed_file',
      '#required' => TRUE,
      '#title' => t('ZIP file containing the files referenced in the source data'),
      '#size' => 48,
      '#description' => t('Select a file to upload.<br/>Files must be less than <strong>@size MB.</strong>', array('@size' => $upload_size)),
      '#default_value' => isset($form_state['values']['file3']) ? $form_state['values']['file3'] : NULL,
      '#upload_location' => file_default_scheme() . '://',
      '#upload_validators' => array(
        'file_validate_extensions' => array('zip'),
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );
 
  return $form;
}

/**
 * ZIP Plupload sub-form for islandora_multi_importer_upload_form.
 */
function islandora_multi_importer_zip_plupload_form($form, &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $form = array(
    'file2' => array(
      '#type' => 'plupload',
      '#required' => TRUE,
      '#title' => t('ZIP file containing the files referenced in the source data'),
      '#plupload_settings' => array(
        'runtimes' => 'html5', // Also consider the order of technologies.
        'chunk_size' => '1mb',
        'unique_names' => TRUE,
        'max_file_count' => 1,
      ),
      '#description' => t('Select a file to upload.<br/>Files must be less than <strong>@size MB.</strong>', array('@size' => $upload_size)),
      '#default_value' => isset($form_state['values']['file3']) ? $form_state['values']['file3'] : NULL,
      '#upload_location' => file_default_scheme() . '://',
      '#upload_validators' => array(
        'file_validate_extensions' => array('zip'),
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );
  $form['file2']['#element_validate'][] = 'islandora_multi_importer_plupload_element_validate';
  return $form;
}

/**
 * ZIP Plupload element file formatter disguised as validator.
 */
function islandora_multi_importer_plupload_element_validate($element, &$form_state) {
  $saved_file = 0; // means no fid;
  $scheme = file_default_scheme() . '://';
  foreach ($form_state['values']['last']['file2'] as $uploaded_file) {
    if ($uploaded_file['status'] == 'done') {
      $source = $uploaded_file['tmppath'];
      $destination = file_stream_wrapper_uri_normalize($scheme . "imi_".time()."_".$uploaded_file['name']);
      $destination = file_unmanaged_move($source, $destination, FILE_EXISTS_RENAME);
      $file = plupload_file_uri_to_object($destination);
      file_save($file);
      // Yes, we do a single one only.
      $saved_file = $file->fid; 
    }
  }
  $form_state['values']['last']['file2'] =  $saved_file;
}


function islandora_multi_importer_choosesource_form($form, &$form_state) {
  $form =  array(
    'data_source' => array(
      '#prefix' => '<div class="help" id="islandora-multi-source">',
      '#suffix' => '</div>',
      '#type' => 'select',
      '#title' => t('Choose your Data source type'),
      '#options' => array('file' => 'Spreadsheet File to be uploaded','google' => 'Google Sheet URL'),
      '#default_value' => 'file',
    ),
  );
  return $form;
}

/**
 * Spreadsheet previsualization form.
 */
function islandora_multi_importer_previs_form(array $form, array &$form_state, array &$file_data) {

  $per_page = 20;
  $previous_page = isset($form_state['storage']['values']['general']['previous_page']) ? $form_state['storage']['values']['general']['previous_page'] : 0;
  $page = isset($form_state['values']['step3']['sheet_content']['page']) ? $form_state['values']['step3']['sheet_content']['page'] : 0;
  
  // Only if data is empty or previous page different to current
  // run this expensive code.
  if (($page != $previous_page) || empty($file_data)) {  
    //Two options here really.
    // A) if google sheet id is set and range is set, try there first,
    // B) if not go for the file
    
    $file_data = islandora_multi_importer_datasource_loader($form_state, $page, $per_page);
    
    $form_state['storage']['values']['general']['previous_page'] = $page;
    $form_state['storage']['values']['general']['file_data'] = $file_data;
  }

  // Multistep form gets broken by pager. Lets make a simple drop down.
  $numpages = ceil($file_data['totalrows'] / $per_page);
  $pages = range(1, $numpages);
  $offset_notice = $page * $per_page + $per_page;
  $showingof = $offset_notice < $offset_notice ? $file_data['totalrows'] : $offset_notice;
  

  $form['sheet_content'] = array(
    '#type' => 'fieldset',
    '#prefix' => '<div class="help">',
    '#suffix' => '</div>',
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#tree' => TRUE,
    '#title' => t('Preview submited file data values'),
    'rows' => array(
      '#prefix' => '<div class="help" style="overflow:scroll" id="islandora-multi-preview">',
      '#suffix' => t('Showing records from @num to @offset of @totalrows', array(
        '@num' => $page * $per_page,
        '@offset' => $showingof ,
        '@totalrows' => $file_data['totalrows'],
      )) . '</div>',
      '#type' => 'tableselect',
      '#header' => $file_data['headers'],
      '#multiple' => FALSE,
      '#options' => $file_data['data'],
      '#js_select' => TRUE,
      '#attributes' => array(
        'class' => array(
          'tableheader-processed',
        ),
      ),
      
    ),
    'page' => array(
      '#type' => 'select',
      '#title' => t('Pages'),
      '#maxlength' => 3,
      '#options' => $pages,
      '#default_value' => $page ,
      '#ajax' => array(
        'callback' => 'islandora_multi_importer_previs_ajax',
        'wrapper' => 'islandora-multi-preview',
      ),
    ),
    'userow' => array(
      '#type' => 'submit',
      '#value' => t('Use selected row'),
      '#ajax' => array(
        'callback' => 'islandora_multi_importer_ajax_updaterecordpreview',
        'wrapper' => 'islandora-multi-preview',
      ),
    ),
  );
  // Note: islandora_multi_importer_ajax_updaterecordpreview is in callbacks.inc file.
  // not here, don´t search for it!
  return $form;
}

/**
 * Twig edit/preview Form.
 */
function islandora_multi_importer_twig_form($form, &$form_state, $file_data) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  $form['preview_data'] = array(
    '#type' => 'fieldset',
    '#name' => 'islandora-multi-rows-preview',
    '#id' => drupal_html_id('islandora-multi-rows-preview'),
    '#prefix' => '<div class="help" id="islandora-multi-rows-preview-replace">',
    '#suffix' => '</div>',
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#tree' => TRUE,
    '#title' => t('Selected Data Row for template testing'),
  );

  if (isset($form_state['values']['step3']['sheet_content'])) {
    $current_row = isset($form_state['values']['step3']['sheet_content']['rows']) && 
      !empty($form_state['values']['step3']['sheet_content']['rows']) ?  $form_state['values']['step3']['sheet_content']['rows'] : key($file_data['data']);
    $current_record_record_data = array('data' => islandora_multi_importer_array_combine_special($file_data['headers'], $file_data['data'][$current_row]));

    
  }
  else {
    if ((!empty($file_data)) and !empty($file_data['headers'])) {
      $current_row = key($file_data['data']);
      $current_record_record_data = array('data' => islandora_multi_importer_array_combine_special($file_data['headers'], $file_data['data'][$current_row]));
      
    }
    else {
      // We should be never get here. If we are here our file has not data? WE need to push the user back to another step
      $current_record_record_data = array('name' => 'metro.org');
      
      if ($form_state['storage']['values']['step0']['data_source'] == 'google') {
        drupal_set_message(t('Seems like your Google Spreadsheet has no data, no headers or we are having trouble fetching it. Try selecting another one or a different Range.'),'warning');
       
      }
      else {
        drupal_set_message(t('Seems like your File has no data, no headers or we are having trouble fetching it. Try selecting another file.'),'warning');
        
      }
    }
  }
  
  
  // Create a small sub table so Template developer
  // can see the data used to test template.
  $form['preview_data']['table']['#markup'] = theme('table', array('header' => $file_data['headers'], 'rows' => array($file_data['data'][$current_row])));

  $twig_name = isset($form_state['values']['step4']['group_tabs']['group_twig_tab']['savemenu']['template_chooser']['template_list']) ? $form_state['values']['step4']['group_tabs']['group_twig_tab']['savemenu']['template_chooser']['template_list'] : FALSE;
  if ($twig_name) {
    unset($form_state['input']['step4']['group_tabs']['group_twig_tab']['twig']);
    $twig_template_toload = islandora_multi_importer_twig_fetch($twig_name);
  }
  $twig_template_default = isset($twig_template_toload) ? $twig_template_toload : '{% block content %} Hello {{ name }}{% endblock %}';
  $twig_name_default = isset($twig_name) ? $twig_name : t('New Twig template');
  $twig_template = isset($form_state['values']['step4']['group_tabs']['group_twig_tab']['twig']) ? $form_state['values']['step4']['group_tabs']['group_twig_tab']['twig'] : $twig_template_default;
  $loaderFile = new Twig_Loader_Filesystem(__DIR__ . '/../templates');
  $loaderArray = new Twig_Loader_Array(array(
    'forminput.xml' => $twig_template,
  ));

  $loader = new Twig_Loader_Chain(array($loaderFile, $loaderArray));
  $twig = new \Twig_Environment($loader, array(
    'cache' => drupal_realpath('private://'),
    'debug' => TRUE,
  ));
  $twig->addExtension(new Twig_Extension_Debug());

  $validated = TRUE;
  try {
    // The $template is valid.
    $twig->parse($twig->tokenize($twig_template));
  }
  catch (Exception $e) {
    // TODO: We should encourage this to be as explicit
    // to final user as possible.
    error_log(get_class($e));
  }
  catch (Twig_Error_Runtime $e) {
    $validated = FALSE;
    // In case there is a previous exception, re-throw the previous exception,
    // so that the original exception is shown, rather than
    // \Twig_Template::displayWithErrorHandling()'s exception.
    $previous_exception = $e->getPrevious();
    if ($previous_exception) {
      drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error' => $previous_exception->getMessage())), 'error');
    }
    drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error' => $e->getRawMessage())), 'error');
    $output = '';
  }
  catch (Twig_Error_Syntax $e) {
    // $template contains one or more syntax errors.
    $validated = FALSE;
    drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error' => $e->getRawMessage())), 'error');
    $output = '';
  }

  if ($validated) {
    try {
      // The $template syntax is valid, but still can throw run time errors.
      $template = $twig->createTemplate($twig_template);
      $output = $template->render($current_record_record_data);
      //Remove those nasty emtpy lines...
      // @TODO add a toogle for this?
      $output = preg_replace("/(^[\r\n]*|[\r\n]+)[\s\t]*[\r\n]+/", "\n", $output);
    }
    catch (Twig_Error_Runtime $e) {
      $validated = FALSE;
      // In case there is a previous exception, re-throw the previous exception,
      // so that the original exception is shown, rather than
      // \Twig_Template::displayWithErrorHandling()'s exception.
      $previous_exception = $e->getPrevious();
      if ($previous_exception) {
        drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error' => $previous_exception->getMessage())), 'error');
      }
      drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error' => $e->getRawMessage())), 'error');
      $output = '';
    }
  }

  $form['#attached']['css'] = array(
    drupal_get_path('module', 'field_group') . '/horizontal-tabs/horizontal-tabs.css',
  );
  $form['#attached']['js'] = array(
    drupal_get_path('module', 'field_group') . '/horizontal-tabs/horizontal-tabs.js',
  );

  // Horizontal tabs container.
  $form['group_tabs'] = array(
    '#type' => 'horizontal_tabs',
    '#tree' => TRUE,
  );
  // My Twig template Tab.
  $form['group_tabs']['group_twig_tab'] = array(
    '#type'  => 'fieldset',
    '#title' => t('Twig Template Input'),
    '#group' => 'group_tabs',
  );

  // Field - Twig Input.
  $form['group_tabs']['group_twig_tab']['twig'] = array(
    '#prefix' => '<div id="islandora-multi-twigfull">',
    '#suffix' => '</div>',
    '#type' => 'textarea',
    '#title' => $twig_name_default,
    '#description' => t("All information is stored in the 'data' object. Example: to reference a column named identifier use <em>data.identifier</em>, if the column has special characters that can not be parsed as a PHP variable use this notation: <em>attribute(data, 'identifier-with-dash') </em>. More info at !url", array('!url' => l(t('twig.sensiolabs.org'), "http://twig.sensiolabs.org/doc/templates.html#variables"))),
    '#group' => 'group_twig_input',
    '#rows' => 20,
    '#default_value' => $twig_template_default,
  );

  // Field - submit button.
  $form['group_tabs']['group_twig_tab']['submitpreview'] = array(
    '#prefix' => '<div style="height:50px">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    '#type' => 'submit',
    '#value' => t('Preview'),
    '#group' => 'group_tabs',
    '#ajax' => array(
      'callback' => 'islandora_multi_importer_previstwig_ajax',
      'wrapper' => 'islandora-multi-twigoutput',
    ),
  );
  $form['group_tabs']['group_twig_tab']['savemenu'] = array(
    '#prefix' => '<div class="form-group" id="islandora-multi-twigsave">',
    '#suffix' => '</div>',
    '#title' => t('Manage your templates'),
    '#tree' => TRUE,
    '#type' => 'fieldset',
  );

  $form['group_tabs']['group_twig_tab']['savemenu']['presave'] = array(
    '#prefix' => '<div style="height:50px;">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    '#type' => 'button',
    '#value' => t('Save Template As'),
    '#group' => 'group_tabs',
    '#access' => ($form_state['triggering_element']['#value'] != t('Save Template As')),
    '#ajax' => array(
      'callback' => 'islandora_multi_importer_savetwig_ajax',
      'wrapper' => 'islandora-multi-twigsave',
    ),
  );

  // This one is only displayed if the save button was pressed.
  $form['group_tabs']['group_twig_tab']['savemenu']['save'] = array(
    '#prefix' => '<div class="form-group" id="islandora-multi-twigsave">',
    '#suffix' => '</div>',
    '#access' => ($form_state['triggering_element']['#value'] == t('Save Template As')),
    '#tree' => TRUE,
    '#group' => 'group_tabs',
    'template_name' => array(
      '#type' => 'textfield',
      '#description' => 'Name for your new template',
      '#default_value' => 'My New Template',
    ),
    'realsave' => array(
      '#tree' => TRUE,
      '#type' => 'submit',
      '#value' => t('Save Template'),
      '#ajax' => array(
        'callback' => 'islandora_multi_importer_savefinaltwig_ajax',
        'wrapper' => 'islandora-multi-twigsave',
      ),
    ),
    'cancel' => array(
      '#tree' => TRUE,
      '#type' => 'button',
      '#value' => t('Cancel'),
      '#ajax' => array(
        'callback' => 'islandora_multi_importer_savefinaltwig_ajax',
        'wrapper' => 'islandora-multi-twigsave',
      ),
    ),
  );

  $form['group_tabs']['group_twig_tab']['savemenu']['template_chooser'] = array(
    '#prefix' => '<div class="help" id="islandora-multi-twigchooser"></div>',
    '#type' => 'fieldset',
    '#title' => 'Load existing template',
    'template_list' => array(
      '#type' => 'select',
      '#multiple' => FALSE,
      '#description' => t('Stored templates'),
      '#empty_option' => t('- Select -'),
      '#options' => islandora_multi_importer_twig_list(),
      '#ajax' => array(
        'callback' => 'islandora_multi_importer_loadtwig_ajax',
        'wrapper' => 'islandora-multi-twigfull',
      ),
    ),
  );

  /* if ($form_state['clicked_button']['#value'] == 'Save Template') {
  $form['group_tabs']['group_twig_tab']['template_name']['#value'] = 'saved';
  }*/

  // My Parsed Output tab.
  $form['group_tabs']['group_twigoutput_tab'] = array(
    '#type'  => 'fieldset',
    '#title' => t('Template Parsed output'),
    '#group' => 'group_tabs',
  );

  // Field - Twig Output.
  $form['group_tabs']['group_twigoutput_tab']['twig'] = array(
    '#prefix' => '<div class="help" id="islandora-multi-twigoutput"><pre>',
    '#suffix' => '</pre></div>',
    '#title' => t('Template Parsed Output'),
    'raw' => array(
      '#title' => t('Template Raw Output'),
      '#type' => 'textarea',
      '#rows' => 50,
      '#value' => $output,
      '#group' => 'group_twig_input',
    ),
    'parsed_and_enriched' => array(
      '#title' => t('Template Parsed And Highlighted'),
      '#group' => 'group_twig_input',
      '#markup' => islandora_multi_importer_xml_highlight($output),
    ),
  );
  return $form;
}

/**
 * Helper function to construct the field headers/mapping table.
 *
 * @param array $form
 *   The parent form.
 * @param array $form_state
 *   The parent form state.
 * @param array $file_data
 *   The tabulated file data.
 *
 * @return array
 *   An array representing the available mappings
 */
function islandora_multi_importer_cmodelmapping_form(array $form, array &$form_state, array $file_data) {

  $options_for_metadata = array();
  $options_for_binary = array();
  $rows = array();
  // Base mappings needed to ingest.
  if (isset($form_state['clicked_button']) && ($form_state['clicked_button']['#value'] == 'Check CMODELS')) {
    $colum_index = $form_state['values']['cmodelmap']['basemapping']['rows']['cmodelmap_row']['cmodelmap'];
    // Sadly we have to fetch all data here.

    $file_data_all = islandora_multi_importer_datasource_loader($form_state, 0, -1);

    $cmodel_colum = array_map(function ($element) use ($colum_index) {
      return $element[$colum_index];
    }, $file_data_all['data']);

    $real_cmodels = islandora_multi_importer_mapper_check_cmodels($cmodel_colum);
    foreach ($real_cmodels as $cmodel => $dsid) {
      $derivatives_available[$cmodel] = islandora_multi_importer_mapper_get_derivatives($cmodel);
    }
    if (count($real_cmodels)) {
      $form_state['storage']['values']['general']['computed_cmodels'] = $real_cmodels;
      $form_state['storage']['values']['general']['computed_derivatives'] = $derivatives_available;
    }
    else {
      drupal_set_message(t('No valid cmodels found, please check spelling for select field value'), 'warning');
      unset($form_state['storage']['values']['general']['computed_cmodels']);
      unset($form_state['storage']['values']['general']['computed_derivatives']);
    }

  }

  $rows = array(
    'cmodelmap_row' => array(
      '#id' => drupal_html_id('islandora-multi-cmodelmap'),
      'cmodelmap' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select the source data field to use for CMODEL mapping'),
        '#options' => $file_data['headers'],
      ),
      'checkcmodels_button' => array(
        '#prefix' => '<div style="height:50px">',
        '#suffix' => '</div>',
        '#tree' => TRUE,
        '#type' => 'submit',
        '#value' => t('Check CMODELS'),
        '#ajax' => array(
          'callback' => 'islandora_multi_importer_mapper_ajax',
          'wrapper' => 'islandora-multi-cmodelmapping',
          'effect' => 'fade',
        ),
      ),
    ),
  );

  $form['basemapping'] = array(
    '#title' => t('CMODEL source field'),
    '#tree' => TRUE,
    '#attributes' => array(
      'class' => 'form-group',
    ),
    'rows' => array(),
  );
  foreach ($rows as $key => $row) {
    $form['basemapping']['rows'][$key] = $row;
  }
  $form['cmodels'] = islandora_multi_importer_cmodeldsmapping_form($form_state, $file_data);
  
  // Allows users to add extra DSIDS not defined in each CMODEL
  
  
  
  return $form;
}

/**
 * Helper function to construct the CMODEL/DS mapping table.
 *
 * @param array $form_state
 *   The parent form state.
 * @param array $file_data
 *   The tabulated file data.
 *
 * @return array
 *   An array representing the available mappings
 */
function islandora_multi_importer_cmodeldsmapping_form(array &$form_state, array $file_data) {
  $form = array(
    '#prefix' => '<div id="islandora-multi-cmodelmapping">',
    '#suffix' => '</div>',
    '#type' => 'markup',
  );
  $rows = array();
  // Check action here.
  $parameters['action'] = isset($form_state['values']['action']) ? $form_state['values']['action'] : 'ingest';
  
  // Only if cmodels where computed previously.
  if (isset($form_state['storage']['values']['general']['computed_cmodels'])) {
    $computed_cmodels = $form_state['storage']['values']['general']['computed_cmodels'];
    // Deal with user-supplied DSDIs
    if (user_access(ISLANDORA_MULTI_IMPORTER_EXTRADS) && variable_get('islandora_multi_importer_extrads', FALSE)) {
       $form['extrads'] = array(
         '#tree' => TRUE,
         '#type' => 'markup',
       );
       $form['extrads']['newdsid'] = array(
         '#prefix' => '<div class="form-group" id="islandora-multi-newdsid">',
         '#suffix' => '</div>',
         '#title' => t('Add custom Datastreams not defined in computed datastreams'),
         '#tree' => TRUE,
         '#type' => 'fieldset',
       );
       
       $form['extrads']['newdsid']['addmenu']['presave'] = array(
         '#prefix' => '<div style="height:50px;">',
         '#suffix' => '</div>',
         '#tree' => TRUE,
         '#type' => 'button',
         '#value' => t('Add an extra DSID'),
         '#access' => ($form_state['triggering_element']['#value'] != t('Add an extra DSID')),
         '#ajax' => array(
           'callback' => 'islandora_multi_importer_adddsid_ajax',
           'wrapper' => 'islandora-multi-newdsid',
         ),
       );

       // This one is only displayed if the Add an extra DSID button was pressed.
       if (isset($form_state['clicked_button']) && ($form_state['clicked_button']['#value'] == 'Add an extra DSID')) {
         $form['extrads']['newdsid']['addmenu']['presave'] = array(
           '#prefix' => '<div style="height:50px;">',
           '#suffix' => '</div>',
           '#tree' => TRUE,
           '#type' => 'button',
           '#value' => t('Add an extra DSID'),
           '#access' => ($form_state['triggering_element']['#value'] != t('Add an extra DSID')),
           '#ajax' => array(
             'callback' => 'islandora_multi_importer_adddsid_ajax',
             'wrapper' => 'islandora-multi-newdsid',
           ),
         );
         $form['extrads']['newdsid']['addmenu']['save'] = array(
           '#prefix' => '<div id="islandora-multi-newdsidsave">',
           '#suffix' => '</div>',
           '#access' => ($form_state['triggering_element']['#value'] == t('Add an extra DSID')),
           '#tree' => TRUE,
           'dsid_name' => array(
             '#type' => 'textfield',
             '#size' => 64,
             '#description' => 'New datastream ID (DSID)',
             '#default_value' => 'DWC',
             '#element_validate' => array('islandora_multi_importer_validate_dsid'),
           ),
           'realsave' => array(
             '#tree' => TRUE,
             '#type' => 'submit',
             '#value' => t('Add this DSID to every CMODEL'),
             '#ajax' => array(
               'callback' => 'islandora_multi_importer_saveextradsid_ajax',
               'wrapper' => 'islandora-multi-cmodelmapping',
             ),
           ),
           'cancel' => array(
             '#tree' => TRUE,
             '#type' => 'button',
             '#value' => t('Cancel'),
             '#ajax' => array(
               'callback' => 'islandora_multi_importer_saveextradsid_ajax',
               'wrapper' => 'islandora-multi-newdsidsave',
             ),
           ),
         );
     }
  }
   if (empty(form_get_errors()) && isset($form_state['clicked_button']) && ($form_state['clicked_button']['#value'] == 'Add this DSID to every CMODEL')) {
        foreach ($computed_cmodels as $cmodel => $dsid) {
          $new_dsdi = trim($form_state['values']['cmodelmap']['cmodels']['extrads']['newdsid']['addmenu']['save']['dsid_name']);
          $computed_cmodels[$cmodel] += array(
            "$new_dsdi" => array(
              'id' => $new_dsdi,
              'mime'=> array(),
              'optional' => 1
            )
          );
        } 
   }

    foreach ($computed_cmodels as $cmodel => $dsids) {
      $form[$cmodel] = array(
        '#type' => 'fieldset',
        '#tree' => TRUE,
        '#title' => $cmodel,
        '#description' => t('Select the Datastream IDs you want to be created using your data as source'),
        'dsid' => array(
          '#theme' => 'islandora_multi_importer_form_table',
          '#header' => array(
            'dsid' => t('DSID'),
            'method' => t('How?'),
          ),
          'rows' => array(),
          '#default_value' => array(),
        ),
      );
      // Remove unwanted DSIDS.
      unset($dsids['RELS-EXT']);
      unset($dsids['RELS-INT']);
      // Shitty how islandora creates this.
      unset($dsids['TECHMD']);

      // Add a prefix to select values, since we are using optgroup labels
      // to distinguish them on ingest.
      $prefix_options = function ($value, $prefix) {
        return $prefix . $value;
      };

      $field_options = array_combine(array_map($prefix_options, array_keys($file_data['headers']), array_fill(0, count($file_data['headers']), "column|")), array_values($file_data['headers']));
      $template_options = islandora_multi_importer_twig_list();
      // Iterate over existing ones
      // Defaults.
      $defaults = array();
      
      // NEW DSID goes here...
      
      
      foreach ($dsids as $dsid => $dsidinfo) {

        // Individual Options for each DSID.
        $options = array();
        // Exception for DC, allow XSLT.
        if ($dsid == 'DC') {
          // @TODO add XLST options and other sources diff than mods
          $options['via XSLT from MODS'] = array('xlst|0' => 'default XSLT');
        }

        if (($dsidinfo['optional'] == TRUE) || ($parameters['action']!= 'ingest')){
          $options['NONE'] = array('' => t("-- Don't Create --"));
        }
        $options['Via a source data field (file path)'] = $field_options;
        // Todo, add other mime types we can create with Twig
        // (RDF, turtle, html, whatever is not binary)
        if (array_intersect($dsidinfo['mime'], array(
          'text/xml',
          'application/xml',
          'rdf/xml',
          'application/rdf+xml',
          'text/plain',
          'text/html',
        )) || empty($dsidinfo['mime'])){
          // In case no template is there yet (first install)?
          if (!empty($template_options)) {
            $options['Use Twig Template'] = array_combine(array_map($prefix_options, $template_options, array_fill(0, count($template_options), "template|")), $template_options);
          }
        }

        // Fetch Derivative options from computed ones (if any)
        if (isset($form_state['storage']['values']['general']['computed_derivatives'][$cmodel])) {
          foreach ($form_state['storage']['values']['general']['computed_derivatives'][$cmodel] as $key => $derivative) {
            // If destination DSID of this derivative matches
            // current DSID add as default option.
            if ($derivative['destination_dsid'] == $dsid) {
              if (!empty($derivative['source_dsid'])) {
                $options['Derivatives'] = array('derivative|0' => 'Build using derivative from ' . $derivative['source_dsid']);
              } 
              else {
                $options['Derivatives'] = array('derivative|0' => 'Build using islandora generated derivative');
              }
            }
          }
        }
        
        
        
        // Check if DS is optional or not, we enforce not optional ones.
        $defaults[$dsid] = !$dsidinfo['optional'];
        $form[$cmodel]['dsid']['rows'][$dsid] = array(
          '#title' => $dsid,
          'dsid' => array('#markup' => $dsid),
          'method' => array(
            'data' => array(
              '#type' => 'select',
              '#attributes' => array(),
              '#description' => t('Select the way you want to build this DS'),
              '#options' => $options,
            ),
          ),
        );
        $form[$cmodel]['dsid']['#default_value'] = $defaults;
      }
    }

  }

  return $form;
}

/**
 * Object properties and behaviour mapping form.
 */
function islandora_multi_importer_objectmapping_form($form, &$form_state, $file_data) {
  $rows = array(
    'pidmap_row' => array(
      '#id' => drupal_html_id('islandora-multi-parentmap'),
      'label' => array(
        '#markup' => t('Object PID'),
      ),
      'pidmap' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select the source field to use as Object PID'),
        '#options' => $file_data['headers'],
      ),
      'pidtype' => array(
        '#type' => 'checkbox',
        '#description' => t('Check to let Islandora build PID for you and ignore the Object PID source fields'),
        '#default_value' => TRUE,
        '#states' => array(
        	'visible' => array(
        		':input[name=action]' => array('value' => 'ingest'),
        	),
        ),
      ),
      'stub' => array(
        '#markup' => '&nbsp;',
      ),
    ),
    'parentmap_row' => array(
      '#id' => drupal_html_id('islandora-multi-parentmap'),
      'label' => array(
        '#markup' => t('Parent Object'),
      ),
    
      'parentmap' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select the source field to use as parent Object'),
        '#options' => $file_data['headers'],
      ),
      'parenttype' => array(
        '#type' => 'checkbox',
        '#description' => t('If value is not a well formed PID, assume it is a row index present in the same data'),
        '#default_value' =>TRUE,
      ),
      'stub' => array(
        '#markup' => '&nbsp;',
      ),
    ),
    'labelmap_row' => array(
      '#id' => drupal_html_id('islandora-multi-labelmap'),
      'label' => array(
        '#markup' => t('Object Label'),
      ),
      'labelmap' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select the source field to use as label for Object'),
        '#options' => $file_data['headers'],
      ),
      'stub' => array(
        '#markup' => '&nbsp;',
      ),
      'stub2' => array(
        '#markup' => '&nbsp;',
      ),
    ),
    'sequencemap_row' => array(
      '#id' => drupal_html_id('islandora-multi-parentmap'),
      'label' => array(
        '#markup' => t('Sequence and Ordering'),
      ),
      'sequencemap' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select the source field to use as Sequence order index in multi child objects'),
        '#options' => $file_data['headers'],
      ),
      'stub1' => array(
        '#markup' => '&nbsp;',
      ),
      'stub2' => array(
        '#markup' => '&nbsp;',
      ),
    ),
    'dsmap_row' => array(
      '#id' => drupal_html_id('islandora-multi-dsmap'),
      'label' => array(
        '#markup' => t('Remote DS sources'),
      ),
      'dsremote' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select public accesible resource location'),
        '#options' => array(
          'REMOTE' => 'AWS S3/Dropbox via http(s)',
          'LOCAL' => 'local*',
          'ZIP' => 'ZIP',
        ),
      ),
    ),
    'relsext_row' => array(
      '#id' => drupal_html_id('islandora-multi-relsext'),
      '#states' => array(
      	'visible' => array(
      		':input[name=action]' => array('value' => 'update'),
      	),
      ),    
      'label' => array(
        '#markup' => t('RELS-EXT behaviour'),
        '#states' => array(
          'visible' => array(
            ':input[name=action]' => array('value' => 'update'),
          ),
        ),
      ),
      'relsext_mode' => array(
        '#type' => 'select',
        '#attributes' => array(),
        '#description' => t('Select the way you would like to handle predicates for your existing objects'),
        '#options' => array(
          'NEW' => 'NEW: Only use new Predicates, nothing gets copied over',
          'MERGE' => 'MERGE: Objects are related to a new parent, but relationships to other types are maintained.',
          'JOIN' => 'JOIN: Objects keep their old relationships, New ones are added too',
          'OLD' => 'OLD: Never touch my existing predicates, new cmodel, properties and new parent objects will be ignored',
        ),
        '#default_value' => 'NEW',
        '#states' => array(
        	'visible' => array(
        		':input[name=action]' => array('value' => 'update'),
        	),
        ),
      ),
      'stub' => array(
        '#markup' => '&nbsp;',
      ),
      'stub2' => array(
        '#markup' => '&nbsp;',
      ),
    ),
  );
  $form['objectmapping'] = array(
    '#prefix' => '<div id="islandora-multi-basemapping">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
       // Custom theme to display form elements in table.
    '#theme' => 'islandora_multi_importer_form_table',
    '#header' => array('', t('Source Field Mapping'), '', ''),
       // Rows in the form table.
    'rows' => array(),
  );
  foreach ($rows as $key => $row) {
    $form['objectmapping']['rows'][$key] = $row;
  }

  return $form;
}

/**
 * Upload callback for ajax driven form.
 */
function islandora_multi_importer_form_upload_callback($form, $form_state) {
  return $form;
}

/**
 * Upload ZIP callback for ajax driven form.
 */
function islandora_multi_importer_form_upload_zip_callback($form, $form_state) {
  return $form;
}

/**
 * Previsualization callback for ajax driven form.
 */
function islandora_multi_importer_previs_ajax($form, &$form_state) {
  return $form['step3']['sheet_content']['rows'];
}

/**
 * Previsualization callback for ajax driven form.
 */
function islandora_multi_importer_changeaction_ajax($form, &$form_state) {
  return $form;
}


/**
 * Twig Previsualization callback for ajax driven form.
 */
function islandora_multi_importer_previstwig_ajax($form, &$form_state) {
  $commands[] = ajax_command_replace('#islandora-multi-twigoutput', render($form['step4']['group_tabs']['group_twigoutput_tab']['twig']));
  /*   'command' => 'invoke',
    'selector' => $selector,
    'method' => $method,
    'arguments' => $arguments,
  */
  $commands[] = array('command' => 'focusHTab', 'arguments' => array('tabid' => '#edit-step4-group-tabs-group-twigoutput-tab'));
  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * Save template callback for ajax driven form.
 */
function islandora_multi_importer_savetwig_ajax($form, &$form_state) {
  return $form['step4']['group_tabs']['group_twig_tab']['savemenu'];
}

/**
 * Final, real save template callback for ajax driven form.
 */
function islandora_multi_importer_savefinaltwig_ajax($form, &$form_state) {
  return $form['step4']['group_tabs']['group_twig_tab']['savemenu'];
}

/**
 * Load saved template callback for ajax driven form.
 */
function islandora_multi_importer_loadtwig_ajax($form, &$form_state) {
  return $form['step4']['group_tabs']['group_twig_tab']['twig'];
}

/**
 * Mapper callback for ajax driven form.
 */
function islandora_multi_importer_mapper_ajax($form, &$form_state) {
  return $form['cmodelmap']['cmodels'];
}


/**
 * Add DSID callback for ajax driven form.
 */
function islandora_multi_importer_adddsid_ajax($form, &$form_state) {
  return $form['cmodelmap']['cmodels']['extrads'];
}

/**
 * Add DSID callback for ajax driven form.
 */
function islandora_multi_importer_saveextradsid_ajax($form, &$form_state) {
  return $form['cmodelmap']['cmodels'];
}

function islandora_multi_importer_validate_dsid($element, &$form_state, $form) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  if (!islandora_multi_importer_isValidXmlName($element['#value'])) {
    form_set_error('cmodelmap][cmodels][extrads][newdsid][addmenu][save][dsid_name','');
  }
}



/**
 * Main submit handler for Multi Importer form.
 */
function islandora_multi_importer_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/islandora_multi_batch');
  module_load_include('inc', 'islandora', 'includes/utilities');
  // Store step data.
  $current_step = 'step' . $form_state['storage']['step'];
  //dpm($form_state['values']);
  //dpm('current step inside submit handler', $current_step);
  if (!empty($form_state['values'][$current_step])) {
    $form_state['storage']['values'][$current_step] = $form_state['values'][$current_step];
  } 
  

  // Increment or decrement the step as needed. Recover values if they exist.
  if ($form_state['triggering_element']['#name'] == 'next') {
    if ($form_state['values'][$current_step]['data_source'] == 'file') {
       $form_state['storage']['step'] = 1;
        $form_state['rebuild'] = TRUE;
     }
     if ($form_state['values'][$current_step]['data_source'] == 'google') {
        $form_state['storage']['step'] = 2;
        $form_state['rebuild'] = TRUE;
      }   
  if (isset($form_state['storage']['values']['general']['file_data']) ) {
    // we need to clear previously loaded source data if we are changing the source data
    unset($form_state['storage']['values']['general']['file_data']);
    }
  }


  
  if ($form_state['triggering_element']['#name'] == 'ini') {
      $form_state['storage']['step'] = 0;
       $form_state['rebuild'] = TRUE;
  }
  if ($form_state['triggering_element']['#name'] == 'back') {
    $form_state['storage']['step'] = 0 ;
    $form_state['rebuild'] = TRUE;


  }
  if ($form_state['triggering_element']['#name'] == 'preprocess') {
    $form_state['storage']['step'] = 3;
    $form_state['rebuild'] = TRUE;

  }
  if ($form_state['triggering_element']['#name'] == 'reread') {
    $form_state['storage']['step'] = 3;
    //clears google data and forces a remote re-read
    unset($form_state['storage']['values']['general']['file_data']);
    $form_state['rebuild'] = TRUE;

  }


  $step_name = 'step' . $form_state['storage']['step'];
  if (!empty($form_state['storage']['values'][$step_name])) {
    $form_state['values'][$step_name] = $form_state['storage']['values'][$step_name];
  }

  if ($form_state['triggering_element']['#value'] == t('Save Template')) {
    $twig_template = trim($form_state['values']['step4']['group_tabs']['group_twig_tab']['twig']);
    $twig_template_name = trim($form_state['values']['step4']['group_tabs']['group_twig_tab']['savemenu']['save']['template_name']);
    if (!empty($twig_template) && !empty($twig_template_name)) {
      // Save the template to db.
      if (islandora_multi_importer_twig_save($twig_template_name, $twig_template)) {
        drupal_set_message(t('Template Saved'));
        drupal_static_reset('islandora_multi_importer_twig_list');
      }
      else {
        drupal_set_message(t('Template Failed to Save, empty content or already existing template with same name'), 'error');
      }
    }
  }

  // Handle the ingest buttom.
  if ($form_state['triggering_element']['#value'] == t('Ingest')) {
    // Check if a ZIP file needs to be uploaded.
    if ($form_state['storage']['step'] == 3) {
      // Lets check if Google was used as source. I should
     
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
        $googledata = islandora_multi_importer_read_googledata($spreadsheetId, $range, -1, 0);
        if ($googledata['totalrows'] == 0 || empty($googledata['headers'])) {
          drupal_set_message(t('Seems like your Google Spreadsheet has no data, no headers or we are having trouble fetching it. We are so sorry.'),'error');
          $form_state['storage']['step'] = 2 ;
          $form_state['rebuild'] = TRUE;
          return $form;
        }
        $form_state['storage']['values']['step1']['file'] = islandora_multi_importer_csv_save($googledata);
      }

      if ($form_state['values']['objectmap']['objectmapping']['rows']['dsmap_row']['dsremote'] == 'ZIP') {
        
        $form_state['storage']['step'] = 4;
        $form_state['storage']['values']['step4'] = array(
          'type' => $form_state['values']['objectmap']['objectmapping']['rows']['dsmap_row']['dsremote'],
          'source_data' => $form_state['storage']['values']['step1']['file'],
          'cmodel_source_field_index' => $form_state['values']['cmodelmap']['basemapping']['rows']['cmodelmap_row']['cmodelmap'],
          'cmodel_mapping' => $form_state['values']['cmodelmap']['cmodels'],
          'object_maping' => $form_state['values']['objectmap']['objectmapping']['rows'],
          'computed_cmodels' => $form_state['storage']['values']['general']['computed_cmodels'],
        );
        $form_state['rebuild'] = TRUE;
        return $form;
      }
      else {
        // @TODO This all needs to be validated
        
        $parameters = array(
          'type' => $form_state['values']['objectmap']['objectmapping']['rows']['dsmap_row']['dsremote'],
          'source_data' => $form_state['storage']['values']['step1']['file'],
          'cmodel_source_field_index' => $form_state['values']['cmodelmap']['basemapping']['rows']['cmodelmap_row']['cmodelmap'],
          'cmodel_mapping' => $form_state['values']['cmodelmap']['cmodels'],
          'object_maping' => $form_state['values']['objectmap']['objectmapping']['rows'],
          'computed_cmodels' => $form_state['storage']['values']['general']['computed_cmodels'],
        );
      }
    }
    else  {
      // Means ZIP was selected, or step is last.
      $parameters = $form_state['storage']['values']['step4'];
      $parameters['source_binaries'] = $form_state['values']['last']['file2'];
    }

    // Default action will be ingest.
    // Actually in IslandoraMultiBatch all defaults to ingest
    // except for "update" which means updating existing objects.
    $parameters['action'] = isset($form_state['values']['action']) ? $form_state['values']['action'] : 'ingest';
    // LAST CHECK. IF ACTION is not ingest, we need to make sure that 
    // NO automatic PID is generated.
    $parameters['object_maping']['pidmap_row']['pidtype'] = $parameters['action'] != 'ingest' ? 0 : $parameters['object_maping']['pidmap_row']['pidtype'];

    // Predicates a la carte. 
    $parameters['object_maping']['relsext_row']['relsext_mode'] = $parameters['action'] == 'ingest' ? 'NEW' : $parameters['object_maping']['relsext_row']['relsext_mode'];

    $connection = islandora_get_tuque_connection();
    // kpr($parameters);
    $preprocessor = new IslandoraMultiBatch($connection, $parameters);
    // Pass the preprocessor off to run.
    $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
    // Extra: Lets make sure the CSV file is marked as used by the set:
    $setID = $preprocessor->getSetId();
    watchdog('islandora_multi_importer', 'Parameters for Set @set <pre>@print_r</pre>', array('@print_r' => print_r( $parameters, TRUE), '@set' => $setID), WATCHDOG_INFO);
    //@TODO: switch needed here in case we are dealing with FULL CRUD and not only new objects
    // Since those will basically run differently.
    $file = file_load($form_state['storage']['values']['step1']['file']);
    $file->status = ~FILE_STATUS_PERMANENT; // Keep the uploaded spreadsheet temporary
    file_save($file);
    file_usage_add($file, 'islandora_batch', 'islandora_batch_set', $setID);
 
    drupal_set_message(t('You are all set( <a href="@setreport">id = @set</a>)!', array('@set' => $setID, '@setreport'=>url('admin/reports/islandora_batch_queue/'.$setID))), 'status');
    
    $theProcessedInfo = $preprocessor->getProcessedObjectsInfo();
    $good_objects = format_plural(count($theProcessedInfo['success']), '%num Objects added successfully to set', '%num Object added successfully to set', array('%num' => count($theProcessedInfo['success'])));
    $bad_objects = format_plural(count($theProcessedInfo['error']), 'Row with issue(s) that could not be translated to Object: %errored', 'Rows with issue(s) that could not be translated to Objects: %errored', array('%errored' => implode(",", $theProcessedInfo['error'])));
    if (isset($theProcessedInfo['fatal']) && !empty($theProcessedInfo['fatal'])) {
      drupal_set_message($theProcessedInfo['fatal'], 'error');
    }
    if (count($theProcessedInfo['error']) > 0) {
      drupal_set_message($bad_objects, 'error');
    }
    if (count($theProcessedInfo['success']) > 0) {
      drupal_set_message($good_objects, 'status');
    }


    // islandora_multi_importer_startprocessing($parameters);
    // $form_state['rebuild'] = TRUE;
    return $form;
  }

  // If not finished keep rebuilding.
  $form_state['rebuild'] = TRUE;
  return $form;
}

/**
 * Experimental sent to drush and ZeroMQ processor.
 */
function islandora_multi_importer_startprocessing($parameters) {
  // module_load_include('inc', 'islandora_multi_importer', 'includes/zeroMQ');
  // module_load_include('inc', 'islandora_multi_importer', 'includes/zmsg');
  // drupal_static_reset('islandora_get_tuque_connection');.
  $connection = islandora_get_tuque_connection();
  $preprocessor = new IslandoraMultiBatch($connection, $parameters);

  // Pass the preprocessor off to run.
  $preprocessed = islandora_batch_handle_preprocessor($preprocessor);

  // exec("/usr/local/bin/drush --user=admin
  // islandora-multi-importer-zeroMQboot > /dev/null &");
  // islandora_multi_importer_client_task();
}
