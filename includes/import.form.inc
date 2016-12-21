<?php

/**
 * @file
 * includes/import.form.inc
 *
 * Functions for generating/validating/submitting the import objects form.
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
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/import.form');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/utilities');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/callbacks');
  
  $step = empty($form_state['storage']['step']) ? 1 : $form_state['storage']['step'];
 
  $form_state['storage']['step'] = $step;
  //kpr($form_state);
  $step_forms = array();
  switch ($step) {
      case 1:
      $step_forms['step1'] = islandora_multi_importer_upload_form($form, $form_state);
      $step_forms['submit'] = array(
          '#type' => 'submit',
          '#value' => t('Preprocess'),
          '#weight' => 100,
        );
      break;
      case 2:
      
      //This keeps our file data around between rebuilds
      $file_data = isset($form_state['storage']['values']['general']['file_data']) ? $form_state['storage']['values']['general']['file_data'] : array();
      $step_forms['maintab']['#type'] = 'horizontal_tabs';
    
      $step_forms['#tree'] = true;
      
      $step_forms['step2'] = islandora_multi_importer_previs_form($form, $form_state, $file_data);
      
      $step_forms['step3'] = islandora_multi_importer_twig_form($form, $form_state, $file_data);
      $step_forms['cmodelmap'] = islandora_multi_importer_cmodelmapping_form($form, $form_state, $file_data);
      $step_forms['objectmap'] = islandora_multi_importer_objectmapping_form($form, $form_state, $file_data);
      $step_forms['step2']['#id'] = drupal_html_id('main-data2');
      $step_forms['step2']['#group'] = 'maintab';
      $step_forms['step2']['#collapsible'] = FALSE;
      $step_forms['step2']['#access'] = TRUE;
      $step_forms['step2']['#type'] = 'fieldset';
      $step_forms['step2']['#title'] = t('Your data');
     
      $step_forms['step3']['#id'] = drupal_html_id('main-data3');
      $step_forms['step3']['#group'] = 'maintab';
      $step_forms['step3']['#collapsible'] = FALSE;
      $step_forms['step3']['#access'] = TRUE;
      $step_forms['step3']['#type'] = 'fieldset';
      $step_forms['step3']['#title'] = t('Templating');
     
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
      
      
      
     
     
      $step_forms['ingest'] = array(
            '#type' => 'submit',
            '#value' => t('Ingest'),
        ); 
      
      //We should only show this one when all required info for the ingest is set
      $step_forms['ingest_option'] = array(
              '#type' => 'checkbox',
              '#value' => FALSE,
              '#title' => t('Use zeroMQ processing'),
              '#description' => t('This sends processing to multiple background workers'),
        );   
      break;
      case 3:
      $step_forms['last'] = islandora_multi_importer_zip_upload_form($form, $form_state);
      $step_forms['ingest'] = array(
            '#type' => 'submit',
            '#value' => t('Ingest'),
        ); 
      
      //We should only show this one when all required info for the ingest is set
      $step_forms['ingest_option'] = array(
              '#type' => 'checkbox',
              '#value' => FALSE,
              '#title' => t('Use zeroMQ processing'),
              '#description' => t('This sends processing to multiple background workers'),
        );  
      break;
    }
    
    $form = $step_forms;
    $form['#tree'] = TRUE;
    //kpr($form);
    
    return $form;
}


function islandora_multi_importer_upload_form($form, &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $form = array(
      'file' => array(
        '#type' => 'managed_file',
        '#required' => TRUE,
        '#title' => t('CSV or TSV file containing the rows to import'),
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
  return $form; 
}

function islandora_multi_importer_zip_upload_form($form, &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $form = array(
      'file2' => array(
        '#type' => 'managed_file',
        '#required' => TRUE,
        '#title' => t('ZIP file containing the files referenced in the source data'),
        '#size' => 48,
        '#description' => t('Select a file to upload.<br/>Files must be less than <strong>@size MB.</strong>', array('@size' => $upload_size)),
        '#default_value' => isset($form_state['values']['file2']) ? $form_state['values']['file2'] : NULL,
        '#upload_location' => file_default_scheme() . '://',
        '#upload_validators' => array(
          'file_validate_extensions' => array('zip'),
          'file_validate_size' => array($upload_size * 1024 * 1024),
        ),
      ),
    );
  return $form; 
}


function islandora_multi_importer_previs_form($form, &$form_state, &$file_data) {

  $per_page = 20;
  $previous_page = isset($form_state['storage']['values']['general']['previous_page']) ? $form_state['storage']['values']['general']['previous_page'] : 0;
  $page = isset($form_state['values']['step2']['file_content']['page']) ? $form_state['values']['step2']['file_content']['page'] : 0;
 
  //Only if data is empty or previous page different to current run this expensive code
  if (($page != $previous_page) || empty($file_data)) {
    $file = file_load($form_state['storage']['values']['step1']['file']);
    $file_path = drupal_realpath($file->uri);
    $file_data = islandora_multi_importer_read_filedata($file_path, $per_page, $page * $per_page);
    $form_state['storage']['values']['general']['previous_page'] = $page;
    $form_state['storage']['values']['general']['file_data'] = $file_data;
  }

  //Multistep form gets broken by pager. Lets make a simple drop down
  $numpages = ceil($file_data['totalrows']/$per_page);
  $pages = range(1, $numpages);
  
  $form['file_content'] = array(
    '#type' => 'fieldset',
    '#prefix' => '<div class="help">',
    '#suffix' => '</div>',
    '#collapsed' => TRUE, 
    '#collapsible' => TRUE,
    '#tree' => TRUE,
    '#title' => t('Preview submited file data values'),
      'rows' => array(
      '#prefix' => '<div class="help" style="overflow:scroll" id="islandora-multi-preview">',
      '#suffix' => t('Showing records from @num to @offset of @totalrows', array('@num' => $page * $per_page, '@offset' => $page * $per_page + $per_page, '@totalrows' => $file_data['totalrows'])).'</div>',
      '#type' => 'tableselect',
      '#header' => $file_data['headers'], 
      '#multiple' => FALSE,
      '#options' => $file_data['data'], 
      '#js_select' => TRUE,
      '#attributes' => array(
        'class' => array(
          'tableheader-processed'
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
  
  return $form;
}
function islandora_multi_importer_twig_form($form, &$form_state, $file_data) { 

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
 
  if (isset($form_state['values']['step2']['file_content'])) {
    $current_row = isset($form_state['values']['step2']['file_content']['rows']) && !empty($form_state['values']['step2']['file_content']['rows']) ? $form_state['values']['step2']['file_content']['rows'] : key($file_data['data']);
    $current_record_record_data = array('data' => array_combine($file_data['headers'],$file_data['data'][$current_row]));
    //Create a small sub table so Template developer can see the data used to test template
    $form['preview_data']['table']['#markup'] = theme('table', array('header' => $file_data['headers'], 'rows' => array($file_data['data'][$current_row])));
  }
  else {
    if (!empty($file_data)) {
      $current_row = key($file_data['data']);
      $current_record_record_data = array('data' => array_combine($file_data['headers'],$file_data['data'][$current_row]));
      $form['preview_data']['table']['#markup'] = theme('table', array('header' => $file_data['headers'], 'rows' => array($file_data['data'][$current_row])));
    }
    else {
    //We should be never get here. If we are here our file has not data?
    $current_record_record_data = array('name' => 'metro.org');
    }
  }
  
  $twig_name = isset($form_state['values']['step3']['group_tabs']['group_twig_tab']['savemenu']['template_chooser']['template_list']) ? $form_state['values']['step3']['group_tabs']['group_twig_tab']['savemenu']['template_chooser']['template_list'] : FALSE;
  if ($twig_name) {
     unset($form_state['input']['step3']['group_tabs']['group_twig_tab']['twig']);
    $twig_template_toload = islandora_multi_importer_twig_fetch($twig_name); 
  }
  $twig_template_default = isset($twig_template_toload)? $twig_template_toload :'{% block content %} Hello {{ name }}{% endblock %}';
  $twig_name_default = isset($twig_name) ? $twig_name : t('New Twig template');
  $twig_template = isset($form_state['values']['step3']['group_tabs']['group_twig_tab']['twig']) ? $form_state['values']['step3']['group_tabs']['group_twig_tab']['twig'] : $twig_template_default;
  $loaderFile = new Twig_Loader_Filesystem(__DIR__.'/../templates');
  $loaderArray = new Twig_Loader_Array(array(
    'forminput.xml' => $twig_template,
  ));
  
  $loader = new Twig_Loader_Chain(array($loaderFile, $loaderArray));
  $twig = new \Twig_Environment($loader, array(
    'cache' => drupal_realpath('private://'),
    'debug' => true,
  ));
  $twig->addExtension(new Twig_Extension_Debug());

  $validated = true;
  try {
    // the $template is valid
    $twig->parse($twig->tokenize($twig_template));
  } 
  catch (Exception $e) {
    kpr(get_class($e));
  }
  catch (Twig_Error_Runtime $e) {
     $validated = false;
     // In case there is a previous exception, re-throw the previous exception,
     // so that the original exception is shown, rather than
     // \Twig_Template::displayWithErrorHandling()'s exception.
     $previous_exception = $e->getPrevious();
     if ($previous_exception) {
        drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error'=> $previous_exception->getMessage())),'error');
     }
     drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error'=> $e->getRawMessage())),'error');
    $output = '';
   }
  catch (Twig_Error_Syntax $e) {
    // $template contains one or more syntax errors
    $validated = false;
     drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error'=> $e->getRawMessage())),'error');
    $output = '';
  }

  if ($validated) {
    try {
      // the $template syntax is valid, but still can throw run time errors
        $template = $twig->createTemplate($twig_template);
        $output = $template->render($current_record_record_data);
    } 
    catch (Twig_Error_Runtime $e) {
       $validated = false;
       // In case there is a previous exception, re-throw the previous exception,
       // so that the original exception is shown, rather than
       // \Twig_Template::displayWithErrorHandling()'s exception.
       $previous_exception = $e->getPrevious();
       if ($previous_exception) {
          drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error'=> $previous_exception->getMessage())),'error');
       }
       drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error'=> $e->getRawMessage())),'error');
      $output = '';
     }
  }

  $form['#attached']['css'] = array(
    drupal_get_path('module', 'field_group') . '/horizontal-tabs/horizontal-tabs.css',
  );
  $form['#attached']['js'] = array(
    drupal_get_path('module', 'field_group') . '/horizontal-tabs/horizontal-tabs.js',
  );

  
  // Horizontal tabs container
  $form['group_tabs'] = array(
    '#type' => 'horizontal_tabs',
    '#tree' => TRUE,
  );
  // My Twig template Tab
  $form['group_tabs']['group_twig_tab'] = array(
    '#type'  => 'fieldset',
    '#title' => t('Twig Template Input'),
    '#group' => 'group_tabs',
  );
 
  // Field - Twig Input
  $form['group_tabs']['group_twig_tab']['twig'] = array(
    '#prefix' => '<div id="islandora-multi-twigfull">',
    '#suffix' => '</div>',
    '#type' => 'textarea',
    '#title' => $twig_name_default,
    '#description' => t("All information is stored in the 'data' object. Example: to reference a column named identifier use <em>data.identifier</em>, if the column has special characters that can not be parsed as a PHP variable use this notation: <em>attribute(data, 'identifier-with-dash') </em>. More info at !url", array('!url'=>l("twig.sensiolabs.org", "http://twig.sensiolabs.org/doc/templates.html#variables"))),
    '#group' => 'group_twig_input',
    '#rows' => 20,
    '#default_value' => $twig_template_default,
  );
  
  // Field - submit button
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
 
 
 //This one is only displayed if the save button was pressed
 
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
 )
);

/* if ($form_state['clicked_button']['#value'] == 'Save Template') {
   $form['group_tabs']['group_twig_tab']['template_name']['#value'] = 'saved';
 }*/
 
  // My Parsed Output tab
  $form['group_tabs']['group_twigoutput_tab'] = array(
    '#type'  => 'fieldset',
    '#title' => t('Template Parsed output'),
    '#group' => 'group_tabs',
  );
 
  // Field - Twig Output
  $form['group_tabs']['group_twigoutput_tab']['twig'] = array(
    '#prefix' => '<div class="help" id="islandora-multi-twigoutput"><pre>',
    '#suffix' => '</pre></div>',
    '#title' => t('Template Parsed Output'),
    'raw' => array (
      '#title' => t('Template Raw Output'),
      '#type' => 'textarea',
      '#rows' => 50,
      '#value' => $output,
     '#group' => 'group_twig_input',
    ),
    'parsed' => array(
      '#group' => 'group_twig_input',
      '#markup' => filter_xss($output),
    ),
  );
return $form;
}

/**
 * Helper function to construct the field headers/mapping table.
 *
 * @param array $form
 *   The parent form
 * @param array $form_state
 *   The parent form state
 * @param array $file_data
 *   The tabulated file data 
 *
 * @return array
 *   An array representing the available mappings
 */
function islandora_multi_importer_cmodelmapping_form($form, &$form_state, $file_data) { 
    
    $options_for_metadata = array();
    $options_for_binary = array();
    $rows = array();
    //Base mappings needed to ingest
    if (isset($form_state['clicked_button']) && ($form_state['clicked_button']['#value'] == 'Check CMODELS')) {
      $colum_index = $form_state['values']['cmodelmap']['basemapping']['rows']['cmodelmap_row']['cmodelmap'];
      //Sadly we have to fetch all data here
      $file = file_load($form_state['storage']['values']['step1']['file']);
      $file_path = drupal_realpath($file->uri);
      $file_data_all = islandora_multi_importer_read_filedata($file_path, -1, 0);
      $cmodel_colum = array_map(function($element) use($colum_index){return $element[$colum_index];}, $file_data_all['data']);
      
      $real_cmodels = islandora_multi_importer_mapper_check_cmodels($cmodel_colum);
      foreach ($real_cmodels as $cmodel => $dsid) {
        $derivatives_available[$cmodel] = islandora_multi_importer_mapper_get_derivatives($cmodel);
      }
      if (count($real_cmodels)) {
          $form_state['storage']['values']['general']['computed_cmodels'] = $real_cmodels;
          $form_state['storage']['values']['general']['computed_derivatives'] = $derivatives_available;
      } 
      else {
        drupal_set_message(t('No valid cmodels found, please check spelling for select field value'),'warning');
      }
      
    }

    $rows = array(
    'cmodelmap_row' => array( 
        '#id' => drupal_html_id('islandora-multi-cmodelmap'),
        'cmodelmap' => array( 
        '#type' => 'select',
        '#attributes'=>'',
        '#description' => t('Select the source data field to use for CMODEL mapping'),
        '#options'=> $file_data['headers'],
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
        'class'=> 'form-group',
      ),
      'rows' => array(),
     );
     foreach ($rows as $key => $row) {
      $form['basemapping']['rows'][$key] = $row;
     }
    $form['cmodels'] = islandora_multi_importer_cmodeldsmapping_form($form_state, $file_data);
    return $form;
}
/**
 * Helper function to construct the field headers/mapping table.
 *
 * @param array $form
 *   The parent form
 * @param array $form_state
 *   The parent form state
 * @param array $file_data
 *   The tabulated file data 
 *
 * @return array
 *   An array representing the available mappings
 */

/**
 * Helper function to construct the CMODEL/DS mapping table.
 *
 * @param array $form
 *   The parent form
 * @param array $form_state
 *   The parent form state
 * @param array $file_data
 *   The tabulated file data 
 *
 * @return array
 *   An array representing the available mappings
 */
function islandora_multi_importer_cmodeldsmapping_form(&$form_state, $file_data) { 
  $form = array(
    '#prefix' => '<div id="islandora-multi-cmodelmapping">',
    '#suffix' => '</div>', 
    '#type' => 'markup',
  );
  $rows = array();
  //Only if cmodels where computed previously
  if (isset($form_state['storage']['values']['general']['computed_cmodels'])) {
    $computed_cmodels = $form_state['storage']['values']['general']['computed_cmodels'];
     
    foreach($computed_cmodels as $cmodel => $dsids) {
        
      $form[$cmodel] =  array(
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
      //Remove unwanted DSIDS      
      unset($dsids['RELS-EXT']);
      unset($dsids['RELS-INT']);
      unset($dsids['TECHMD']); // Shitty how islandora creates this
      

      // Add a prexi to select values, since we are using optgroup labels
      // to distinguish them on ingest
      $prefix_options = function($value, $prefix) {
        return $prefix.$value;
      };
    
      $field_options = array_combine(array_map($prefix_options, array_keys($file_data['headers']), array_fill(0, count($file_data['headers']),"column|")), array_values($file_data['headers']));
      $template_options = islandora_multi_importer_twig_list();
      //Iterate over existing ones
      
      $defaults = array(); // Defaults 
      foreach ($dsids as $dsid => $dsidinfo) {
        
        $options = array(); // Individual Options for each DSID
        //Exception for DC, allow XSLT
        if ($dsid == 'DC') {
            //@TODO add XLST options and other sources diff than mods
             $options['via XSLT from MODS']= array('xlst|0' => 'default XSLT');
          }
       
        if ($dsidinfo['optional'] == TRUE) {
           $options['NONE'] = array('' => t("-- Don't Create --"));
        }
        $options['Via a source data field (file path)']= $field_options;
        //Todo, add other mime types we can create with Twig (RDF, turtle, html, whatever is not binary)
       
         if (array_intersect($dsidinfo['mime'], array('text/xml','application/xml','rdf/xml','application/rdf+xml','text/plain'))) {
           //in case no template is there yet (first install)?
          if (!empty($template_options)) {
            $options['Use Twig Template']= array_combine(array_map($prefix_options, $template_options, array_fill(0, count($template_options),"template|")), $template_options);
          }
        }
        
        //Fetch Derivative options from computed ones (if any)
        if (isset($form_state['storage']['values']['general']['computed_derivatives'][$cmodel]))
        {
          foreach($form_state['storage']['values']['general']['computed_derivatives'][$cmodel] as $key => $derivative) {
            //If destination DSID of this derivative matches current DSID add as default option 
            if ($derivative['destination_dsid'] == $dsid) {
              $options['Derivatives'] = array('derivative|0' => 'Build using derivative from '.$derivative['source_dsid']);
            }
          }
        }
        //Check if DS is optional or not, we enforce not optional ones
        
        $defaults[$dsid] = !$dsidinfo['optional'];
        $form[$cmodel]['dsid']['rows'][$dsid] = array(
          '#title' => $dsid,
          'dsid' => array ('#markup' => $dsid ),
          'method' => array( 
            'data' =>array(
            '#type' => 'select',
            '#attributes'=>'',
            '#description' => t('Select the way you want to build this DS'),
            '#options'=> $options,
          ),
        ),
        );
       $form[$cmodel]['dsid']['#default_value'] = $defaults;
      }
    }

  }
 
  return $form;
}



function islandora_multi_importer_objectmapping_form($form, &$form_state, $file_data) { 
  $rows = array(
    'pidmap_row' => array( 
      '#id' => drupal_html_id('islandora-multi-parentmap'),
      'label' => array(
        '#markup' => t('Object PID'),
        ), 
      'pidmap' => array( 
        '#type' => 'select',
        '#attributes'=>'',
        '#description' => t('Select the source field to use as Object PID'),
        '#options'=> $file_data['headers'],
      ),
      'pidtype' => array( 
        '#type' => 'checkbox',
        '#description' => t('Check to let Islandora build PID for you'),
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
        '#attributes'=>'',
        '#description' => t('Select the source field to use as parent Object'),
        '#options'=> $file_data['headers'],
      ),
      'parenttype' => array( 
        '#type' => 'checkbox',
        '#description' => t('If value is not a well formed PID, assume it is a row index present in the same data'),
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
        '#attributes'=>'',
        '#description' => t('Select the source field to use as label for Object'),
        '#options'=> $file_data['headers'],
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
        '#attributes'=>'',
        '#description' => t('Select the source field to use as Sequence order index in multi child objects'),
        '#options'=> $file_data['headers'],
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
          '#attributes'=>'',
          '#description' => t('Select public accesible resource location'),
          '#options'=> array('REMOTE' => 'AWS S3/Dropbox via http(s)','LOCAL' => 'local*','ZIP'=>'ZIP'),
      ),
    ),
  );
    $form['objectmapping'] = array(
      '#prefix' => '<div id="islandora-multi-basemapping">',
      '#suffix' => '</div>',
      '#tree' => TRUE,
       // Custom theme to display form elements in table
       '#theme' => 'islandora_multi_importer_form_table',
       '#header' => array(t(''),t('Source Field Mapping'),t(''),t('')),
       // Rows in the form table.
       'rows' => array(
       ),
     );
     foreach ($rows as $key => $row) {
      $form['objectmapping']['rows'][$key] = $row;
     }
   
    return $form;
}




function islandora_multi_importer_form_upload_callback($form, $form_state) {
  return $form;
}

function islandora_multi_importer_form_upload_zip_callback($form, $form_state) {
  return $form;
}
function islandora_multi_importer_previs_ajax($form, &$form_state) {
  return $form['step2']['file_content']['rows'];
}
function islandora_multi_importer_previstwig_ajax($form, &$form_state) {
  return $form['step3']['group_tabs']['group_twigoutput_tab']['twig'];
}

function islandora_multi_importer_savetwig_ajax($form, &$form_state) { 
  return $form['step3']['group_tabs']['group_twig_tab']['savemenu'];
}
function islandora_multi_importer_savefinaltwig_ajax($form, &$form_state) { 
  return $form['step3']['group_tabs']['group_twig_tab']['savemenu'];
}
function islandora_multi_importer_loadtwig_ajax($form, &$form_state) { 
  return $form['step3']['group_tabs']['group_twig_tab']['twig'];
}

function islandora_multi_importer_mapper_ajax($form, &$form_state) { 
  return $form['cmodelmap']['cmodels'];
}
function islandora_multi_importer_form_submit($form, &$form_state) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/islandora_multi_batch');
  module_load_include('inc', 'islandora', 'includes/utilities');
  // Store step data
  $current_step = 'step' . $form_state['storage']['step'];
  if (!empty($form_state['values'][$current_step])) {
    $form_state['storage']['values'][$current_step] = $form_state['values'][$current_step];
  }

  // Increment or decrement the step as needed. Recover values if they exist.
  if ($form_state['triggering_element']['#value'] == t('Preprocess')) {
   $form_state['storage']['step'] = 2;
  }
 
   $step_name = 'step' . $form_state['storage']['step'];
    if (!empty($form_state['storage']['values'][$step_name])) {
      $form_state['values'][$step_name] = $form_state['storage']['values'][$step_name];
    }
  if ($form_state['triggering_element']['#value'] == t('Save Template')) {
    $twig_template = trim($form_state['values']['step3']['group_tabs']['group_twig_tab']['twig']);
    $twig_template_name = trim($form_state['values']['step3']['group_tabs']['group_twig_tab']['savemenu']['save']['template_name']);
    if (!empty($twig_template) && !empty($twig_template_name)) {
      //Save the template to db
      if (islandora_multi_importer_twig_save($twig_template_name, $twig_template)) {
        drupal_set_message(t('Template Saved'));
        drupal_static_reset('islandora_multi_importer_twig_list');
      }
      else {
        drupal_set_message(t('Template Failed to Save, empty content or already existing template with same name'),'error');
      }
    }
  }

  // Handle the ingest buttom
  if ($form_state['triggering_element']['#value'] == t('Ingest')){
    // Check if a ZIP file needs to be uploaded
    if ($form_state['storage']['step'] == 2) {
      if ($form_state['values']['objectmap']['objectmapping']['rows']['dsmap_row']['dsremote'] == 'ZIP') {
        $form_state['storage']['step'] = 3;
        $form_state['storage']['values']['step3'] = array(
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
    else {
      $parameters = $form_state['storage']['values']['step3'];
      $parameters['source_binaries'] = $form_state['values']['last']['file2'];
    }
   
    $connection = islandora_get_tuque_connection();
    //kpr($parameters);
    $preprocessor = new IslandoraMultiBatch($connection, $parameters);
    // Pass the preprocessor off to run.
    $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
    //islandora_multi_importer_startprocessing($parameters);
    $form_state['rebuild'] = TRUE;
    return $form;
  }

  // If not finished keep rebuilding
  $form_state['rebuild'] = TRUE;
  return $form;
}

function islandora_multi_importer_startprocessing($parameters) {
  //module_load_include('inc', 'islandora_multi_importer', 'includes/zeroMQ');
  //module_load_include('inc', 'islandora_multi_importer', 'includes/zmsg');
  
  
  //drupal_static_reset('islandora_get_tuque_connection');
  $connection = islandora_get_tuque_connection();
  $preprocessor = new IslandoraMultiBatch($connection, $parameters);
  
  // Pass the preprocessor off to run.
  $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
  
  //exec("/usr/local/bin/drush --user=admin islandora-multi-importer-zeroMQboot > /dev/null &");
  //islandora_multi_importer_client_task();
}
