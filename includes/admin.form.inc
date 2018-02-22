<?php

/**
 * @file
 * The admin form for the islandora fits module.
 */

/**
 * Implementation of a form. 
 *
 * @see drupal_get_form
 */
function islandora_multi_importer_admin_form($form, $form_state) {
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/admin.form');
  form_load_include($form_state, 'inc', 'islandora_multi_importer', 'includes/googleapi');
  $authcode = '';
  if (isset($form_state['triggering_element'])) {
    // Textfield AJAX callback.
    if ($form_state['triggering_element']['#name'] == 'islandora_multi_importer_googleapi_authcode') {
      $authcode = $form_state['input']['islandora_multi_importer_googleapi_authcode'];
    }
  } 
  
  $form['islandora_multi_importer_wrapper'] = array(
    '#prefix' => '<div id="islandora_multi_importer_wrapper">',
    '#suffix' => '</div>',
  );
  $form['islandora_multi_importer_generalsettings_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Multi Importer general settings'),
  );
  $form['islandora_multi_importer_generalsettings_fieldset']['islandora_multi_importer_plupload'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use Plupload for ZIP file'),
    '#description' => t('Use Plupload, requires the Drupal plupload module and Library enabled.'),
    '#default_value' => variable_get('islandora_multi_importer_plupload', FALSE),
  );
    
    
  $form['islandora_multi_importer_googleapi_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Google API OAuth2 Credentials'),
    '#description' => t('For more info see Google Developers OAuth2 settings <a href="@google-dev">Website</a>.', array('@google-dev' => 'https://console.developers.google.com/')),
  );
  $form['islandora_multi_importer_googleapi_fieldset']['islandora_multi_importer_googleClientID'] = array(
   '#type' => 'textfield',
   '#title' => t('Google API App ID'),
   '#default_value' => variable_get('islandora_multi_importer_googleClientID', 'VALID_APP_ID'),
   '#required' => FALSE,
 );
 
$passexist = variable_get('islandora_multi_importer_googleClientSecret', 0);


 $form['islandora_multi_importer_googleapi_fieldset']['islandora_multi_importer_googleClientSecret'] = array(
  '#type' => 'password',
  '#title' => t('Google API App Secret Key'),
  '#description' => t("Leave as is if you want to keep the existing value"),
  '#required' => !isset($passexist),
);

if ($passexist) {
  // This will display only if authentication worked out (after, saving)
 $client = islandora_multi_importer_googleclient();
 $authUrl = $client->createAuthUrl(); 
   
 $form['islandora_multi_importer_googleapi_fieldset']['islandora_multi_importer_googleClientAuthCode'] = array(
  '#type' => 'password',
  '#title' => t('Google API Auth Code'),
  '#description' => t('Leave as is if you want to keep the existing value or Enter from <a href="@google-token" target="_blank">this URL</a>.', array('@google-token' => $authUrl)),
  '#required' => !isset($passexist),
);
}


  $form = system_settings_form($form);
  array_unshift($form['#submit'], 'islandora_multi_importer_google_default_password');

  return $form;

}

function islandora_multi_importer_google_default_password($form, &$form_state) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/googleapi');
  if ($form_state['values']['islandora_multi_importer_googleClientSecret'] == '') {
      $form_state['values']['islandora_multi_importer_googleClientSecret'] = variable_get('islandora_multi_importer_googleClientSecret','');
  }
  if (isset($form_state['values']['islandora_multi_importer_googleClientToken']) && $form_state['values']['islandora_multi_importer_googleClientToken'] == '') {
      $form_state['values']['islandora_multi_importer_googleClientToken'] = variable_get('islandora_multi_importer_googleClientToken','');
  }
  $form['buttons']['submit'] = array('#type' => 'submit', '#value' => t('Save Configuration'));
  $form['#submit'] = array('islandora_multi_importer_googleClientSecret', 'islandora_multi_importer_admin_submit_form');

  return $form;
}

function islandora_multi_importer_admin_form_validate($form, &$form_state) {
    $accessToken = false;
    $appid = $form_state['values']['islandora_multi_importer_googleClientID'];
    $secret = !empty($form_state['values']['islandora_multi_importer_googleClientSecret']) ? 
      $form_state['values']['islandora_multi_importer_googleClientSecret'] :
      variable_get('islandora_multi_importer_googleClientSecret','');
      $authCode = !empty($form_state['values']['islandora_multi_importer_googleClientAuthCode']) ? 
      $form_state['values']['islandora_multi_importer_googleClientAuthCode'] :
      variable_get('islandora_multi_importer_googleClientAuthCode', FALSE);
    
      if ($authCode) {
       $client = islandora_multi_importer_googleclient();
       try {
       $accessToken = $client->fetchAccessTokenWithAuthCode($authCode);
       // And we need to activate it
       // Probably by invoking the Sheets API directly
      // Google_Service_Exception: { "error": { "code": 403, "message": "Google Sheets API has not been used in project dmcny-1329 before or it is disabled. Enable it by visiting https://console.developers.google.com/apis/api/sheets.googleapis.com/overview?project=dmcny-1329 then retry. If you enabled this API recently, wait a few minutes for the action to propagate to our systems and retry.", "errors": [ { "message": "Google Sheets API has not been used in project dmcny-1329 before or it is disabled. Enable it by visiting https://console.developers.google.com/apis/api/sheets.googleapis.com/overview?project=dmcny-1329 then retry. If you enabled this API recently, wait a few minutes for the action to propagate to our systems and retry.", "domain": "global", "reason": "forbidden" } ], "status": "PERMISSION_DENIED" } } in Google_Http_REST::decodeHttpResponse() (line 118 of /vagrant/islandora_host/islandora_multi_importer/vendor/google/apiclient/src/Google/Http/REST.php).
       
       
       }
       catch (Exception $e) {
         drupal_set_message(t('@message', array('@message' => check_plain($e->getMessage()))), 'error');
       }
       // Kinda fake, but passes the token to the submit handler without
       // an actual field.
       // This stuff is an array.
       $form_state['values']['islandora_multi_importer_googleClientToken'] = $accessToken;
      }
   
    
    //$checkGoogleAPI = islandora_multi_importer_googlecredentials_validate($appid, $secret, $token);
    if (!$accessToken && $authCode) {
      form_set_error('islandora_multi_importer_googleClientSecret', 'Please make sure your Google API OAuth credentials are correct.');
      form_set_error('islandora_multi_importer_googleClientID', 'Please make sure your Google API OAuth credentials are correct.');
      form_set_error('islandora_multi_importer_googleClientAuthCode', 'Please make sure your Google API OAuth credentials are correct.');
    }
}


/**
 * Form submit handler.
 *
 * @todo Should probably check the fits executable to validate it. 
 * @todo Should we just be using a system settings form instead of this.
 */
function islandora_multi_importer_admin_submit_form($form, &$form_state) {
  foreach ($form_state['values'] as $key => $value) {
    if (is_array($value) && isset($form_state['values']['array_filter'])) {
      $value = array_keys(array_filter($value));
    }
    
    variable_set($key, $value);
  }
  drupal_set_message(t('The settings have been updated!'));
}