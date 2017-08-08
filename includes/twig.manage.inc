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
  $twig_templates_from_db = TwigTemplateDatabase::GetIdsAndNames();

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
        'colspan' => 6,
      ),
    ),
    'rows' => array(),
  );

  foreach ($twig_templates_from_db as $key => $form_name) {
    $name = $form_name;
    $edit = l(t('Edit'), "admin/islandora/twigtemplates/{$key}/duplicate");
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
  
  $form_twigcreate['#tree'] = true;
  $form_twigcreate['#type'] = 'horizontal_tabs';
  $form_twigcreate['source_selector'] = 
 // $form_twigcreate['upload'] = islandora_multi_importer_upload_form(array(), $form_state);
  $form_twigcreate['twig'] = islandora_multi_importer_twig_form(array(), $form_state, $file_data);
  $form_twigcreate['twig']['#id'] = drupal_html_id('main-data3');
  $form_twigcreate['twig']['#group'] = 'maintab';
  $form_twigcreate['twig']['#collapsible'] = FALSE;
  $form_twigcreate['twig']['#access'] = TRUE;
  $form_twigcreate['twig']['#type'] = 'fieldset';
  $form_twigcreate['twig']['#title'] = t('Templating');
 
  return $form + $form_twigcreate;
}
/*
function islandora_multi_importer_twig_form($form, &$form_state, $file_data) {
form_load_include
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

*/