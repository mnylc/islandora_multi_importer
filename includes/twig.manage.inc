<?php

/**
 * @file
 * Callbacks and functions for the Twig template listing Page.
 */

/**
 * Shows Twig template listing.
 *
 * Users can CRUD manage their templates. Heavily borrowed 
 * from XML forms listing.
 *
 * @return array
 *   The table to display.
 */
function islandora_multi_importer_twiglistpage_callback() {

  module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
  $twig_templates_from_db = TwigTemplateDatabase::GetProperties();

  // No forms exist can only create.
  if (count($twig_templates_from_db ) == 0) {
    return '<div>No Twig templagtes are defined. Please create a new form.</div><br/>';
  }

  $table = array(
    'header' => array(
      array(
        'data' => t('Title'),
      ),
      array(
        'data' => t('Operations'),
        'colspan' => 5,
      ),
      array(
        'data' => t('Last Modified'),
      ),
    ),
    'rows' => array(),
  );

  foreach ($twig_templates_from_db as $key => $form_props) {
    $name = $form_props['name'];
    $lastmod = date('r', $form_props['updated']);
    $edit = l(t('Edit'), "admin/islandora/twigtemplates/{$key}/edit");
    $delete = l(t('Delete'), "admin/islandora/twigtemplates/{$key}/delete");
  
    $copy = l(t('Duplicate'), "admin/islandora/twigtemplates/{$key}/duplicate");
    $view = l(t('View'), "admin/islandora/twigtemplates/{$key}/view");
    $export = l(t('Download'), "admin/islandora/twigtemplates/{$key}/download");
    

    $table['rows'][] = array(
      $name,
      $copy,
      $edit,
      $view,
      $export,
      $delete,
      $lastmod,
    );
  }

  return theme('table', $table);
}

/**
 * Creates new Twig template
 *
 * @return $form
 *   the form to deal with CRUDing Twig templaes
 */
function islandora_multi_importer_twigcreate_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/import.form');
  module_load_include('inc', 'islandora_multi_importer', 'includes/import.form');
  
  $file_data = isset($form_state['storage']['values']['general']['file_data']) ? $form_state['storage']['values']['general']['file_data'] : array();
  drupal_add_library('system', 'drupal.ajax');
  drupal_add_library('system', 'jquery.form');
  $form_twigcreate['#tree'] = true;
  $form_twigcreate['#type'] = 'horizontal_tabs';
  $form_twigcreate['twig'] = array();
  $form_twigcreate['twig'] = islandora_multi_importer_twigcreateinternals_form(array(), $form_state, $file_data);
  $form_twigcreate['twig']['#id'] = drupal_html_id('main-data3');
  $form_twigcreate['twig']['#group'] = 'maintab';
  $form_twigcreate['twig']['#collapsible'] = FALSE;
  $form_twigcreate['twig']['#access'] = TRUE;
  $form_twigcreate['twig']['#type'] = 'fieldset';
  $form_twigcreate['twig']['#title'] = t('Templating');
  $form_twigcreate['#attached']['js'][] = drupal_get_path('module', 'islandora_multi_importer') . '/js/utils.js';
 
  return $form_twigcreate;
}

/**
 * Dual purpouse Form, for new and existing Twig Templates.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 * @param array $file_data
 *   An array containing some data for doing previews.
 * @param array $existingtwig
 *   An array with Twig template info, id, content and date. The full package. 
 *
 * @return array
 *   the form to deal with CRUDing Twig templaes
 */
function islandora_multi_importer_twigcreateinternals_form($form, &$form_state, $file_data, $existingtwig = array()) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  global $user;
 
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

  if (isset($form_state['values']['step3']['file_content'])) {
    $current_row = isset($form_state['values']['step3']['file_content']['rows']) && !empty($form_state['values']['step3']['file_content']['rows']) ? $form_state['values']['step3']['file_content']['rows'] : key($file_data['data']);
    $current_record_record_data = array('data' => array_combine($file_data['headers'], $file_data['data'][$current_row]));
    // Create a small sub table so Template developer
    // can see the data used to test template.
    $form['preview_data']['table']['#markup'] = theme('table', array('header' => $file_data['headers'], 'rows' => array($file_data['data'][$current_row])));
  }
  else {
    if (!empty($file_data)) {
      $current_row = key($file_data['data']);
      $current_record_record_data = array('data' => array_combine($file_data['headers'], $file_data['data'][$current_row]));
      $form['preview_data']['table']['#markup'] = theme('table', array('header' => $file_data['headers'], 'rows' => array($file_data['data'][$current_row])));
    }
    else {
      // We should be never get here. If we are here our file has not data?
      $current_record_record_data = array('name' => 'metro.org');
    }
  }


  $twig_template_default = '{% block content %} Hello {{ name }}{% endblock %}';
  $twig_name_default = !empty($existingtwig) ? $existingtwig['name'] : t("New Twig template by @username", array("@username" => $user->name));
  $twig_template = !empty($existingtwig) ? $existingtwig['twig'] : "";
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
   $validated = FALSE;
    // TODO: We should encourage this to be as explicit
    // to final user as possible.
    error_log(get_class($e));
  }
  catch (Twig_Error_Runtime $e) {
    $validated = FALSE;
    error_log("was twig_Error_Runtime");
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
    error_log("was twig_Error_Syntax");
    drupal_set_message(t('Your template has Runtime errors, please review and fix: @error', array('@error' => $e->getRawMessage())), 'error');
    $output = '';
  }

  if ($validated) {
    try {
      // The $template syntax is valid, but still can throw run time errors.
      $template = $twig->createTemplate($twig_template);
      $output = $template->render($current_record_record_data);
      //Remove those nasty empty lines...
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
    '#default_value' => $twig_template,
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
      'callback' => 'islandora_multi_importer_previstwig_edit_ajax',
      'wrapper' => 'islandora-multi-twigoutput-edit',
    ),
  );

  // This one is only displayed if the save button was pressed.
  $form['group_tabs']['group_twig_tab']['save'] = array(
    '#prefix' => '<div class="form-group" id="islandora-multi-twigsave">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    '#group' => 'group_tabs',
    'template_name' => array(
      '#type' => 'textfield',
      '#description' => 'Name for your template',
      '#default_value' =>  $twig_name_default,
    ),
    'realsave' => array(
      '#tree' => TRUE,
      '#type' => 'submit',
      '#value' => t('Save Template'),
    ),
  );

 

  // My Parsed Output tab.
  $form['group_tabs']['group_twigoutput_tab'] = array(
    '#type'  => 'fieldset',
    '#title' => t('Template Parsed output'),
    '#group' => 'group_tabs',
  );

  // Field - Twig Output.
  $form['group_tabs']['group_twigoutput_tab']['twig'] = array(
    '#prefix' => '<div class="help" id="islandora-multi-twigoutput-edit"><pre>',
    '#suffix' => '</pre></div>',
    '#title' => t('Template Parsed Output'),
    'raw' => array(
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
    'parsed_and_enriched' => array(
      '#group' => 'group_twig_input',
      '#markup' => islandora_multi_importer_xml_highlight($output),
    ),
  );
  return $form;
}

function islandora_multi_importer_twigcreate_form_submit($form, &$form_state) {
 module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
 $twig_template = trim($form_state['values']['twig']['group_tabs']['group_twig_tab']['twig']);
 $twig_template_name = trim($form_state['values']['twig']['group_tabs']['group_twig_tab']['save']['template_name']);
 if (!empty($twig_template) && !empty($twig_template_name)) {
   // Save the template to db.
   if (islandora_multi_importer_twig_save($twig_template_name, $twig_template)) {
      drupal_static_reset('islandora_multi_importer_twig_list');
      drupal_set_message(t('A new Twig template named @twig_name has been saved.', array('@twig_name' => $twig_template_name)));
      $form_state['redirect'] = 'admin/islandora/twigtemplates';
    
   }
   else {
     drupal_set_message(t('Template Failed to Save. Please check your fields'), 'error');
   }
 }
 else {
   drupal_set_message(t('Make sure your template and template name are not empty.'), 'error');
 } 
}



/**
 * Edits an existing Twig template
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 * @param int $id
 *   The ID of a Twig template
 *
 * @return array
 *   the form to deal with CRUDing Twig templaes
 */
function islandora_multi_importer_twigedit_form($form, &$form_state, $id) {
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/import.form');
  module_load_include('inc', 'islandora_multi_importer', 'includes/import.form');
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  $file_data = isset($form_state['storage']['values']['general']['file_data']) ? $form_state['storage']['values']['general']['file_data'] : array();
  $form_state['twig_id'] = $id;
  
  // @TODO. and if the %id does not exists? 404? or notice.. not sure
  $thetemplateinfo = islandora_multi_importer_twig_fetchbyid($id);
  if (empty($thetemplateinfo)) {
    drupal_set_message(t('The requested Twig template does not exist'),'warning');
    drupal_not_found();
    drupal_exit();
     
  }
  $form_twigcreate['#tree'] = true;
  $form_twigcreate['#type'] = 'horizontal_tabs';
  $form_twigcreate['twig'] = array();
  $form_twigcreate['twig'] = islandora_multi_importer_twigcreateinternals_form(array(), $form_state, $file_data, $thetemplateinfo);
  $form_twigcreate['twig']['#id'] = drupal_html_id('main-data3');
  $form_twigcreate['twig']['#group'] = 'maintab';
  $form_twigcreate['twig']['#collapsible'] = FALSE;
  $form_twigcreate['twig']['#access'] = TRUE;
  $form_twigcreate['twig']['#type'] = 'fieldset';
  $form_twigcreate['twig']['#title'] = t('Templating');
 
  return $form_twigcreate;
}

function islandora_multi_importer_twigedit_form_submit($form, &$form_state) {
 module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
 $twig_template = trim($form_state['values']['twig']['group_tabs']['group_twig_tab']['twig']);
 $twig_template_name = trim($form_state['values']['twig']['group_tabs']['group_twig_tab']['save']['template_name']);
 $twig_id =  $form_state['twig_id'];
 if (!empty($twig_template) && !empty($twig_template_name) && !empty($twig_id)) {
   // Save the template to db.
   if (islandora_multi_importer_twig_update($twig_id, $twig_template_name, $twig_template)) {
      drupal_static_reset('islandora_multi_importer_twig_list');
      drupal_set_message(t('Your Twig template has been updated.'));
      $form_state['redirect'] = 'admin/islandora/twigtemplates';
    
   }
   else {
     drupal_set_message(t('Template Failed to Save. Please check your fields'), 'error');
   }
 }
 else {
   drupal_set_message(t('Make sure your template and template name are not empty.'), 'error');
 } 
}

/**
 * Deletes a Twig template
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 * @param int $id
 *   The ID of a Twig template
 *
 * @return array
 *   An array representing the confirmation form.
 */
function islandora_multi_importer_twigdelete_form($form, &$form_state, $id) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
  $form_state['twig_id'] = $id;
  $thetemplateinfo = islandora_multi_importer_twig_fetchbyid($id);
  if (empty($thetemplateinfo)) {
    drupal_set_message(t('The requested Twig template does not exist'),'warning');
    drupal_not_found();
    drupal_exit();
     
  }
  $form_state['twig_name'] = $thetemplateinfo['name'];
  return confirm_form($form,
    t('Are you sure you want to delete a twig template named @twig_name?', array('@twig_name' => $form_state['twig_name'])),
    "admin/islandora/twigtemplates",
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Submit handler for the islandora_solr_metadata delete confirm_form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_multi_importer_twigdelete_form_submit($form, &$form_state) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  module_load_include('inc', 'islandora_multi_importer', 'includes/TwigTemplateDatabase');
  if (isset($form_state['twig_id'])) {
    islandora_multi_importer_twig_deletebyid($form_state['twig_id']);
    $form_state['redirect'] = 'admin/islandora/twigtemplates';
    drupal_set_message(t('The Twig template formerly known as @twig_name has been deleted!', array('@twig_name' => $form_state['twig_name'])));
  }
}

/**
 * Twig Previsualization callback for ajax driven form.
 */
function islandora_multi_importer_previstwig_edit_ajax($form, &$form_state) {
  dpm($form['twig']['group_tabs']['group_twigoutput_tab']['twig']);
  $commands[] = ajax_command_replace('#islandora-multi-twigoutput-edit', render($form['twig']['group_tabs']['group_twigoutput_tab']['twig']));
  /*   'command' => 'invoke',
    'selector' => $selector,
    'method' => $method,
    'arguments' => $arguments,
  */
  $commands[] = array('command' => 'focusHTab', 'arguments' => array('tabid' => '#edit-twig-group-tabs-group-twigoutput-tab'));
  return array('#type' => 'ajax', '#commands' => $commands);
}

