<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * hook_islandora_multi_importer_remote_file_get($url, $pid)
 *
 * Addresses https://github.com/mnylc/islandora_multi_importer/issues/41
 *
 * @param string $url
 *   Path to target file
 * @param string $pid
 *   PID of the object the target file is intended for
 *
 * @return array
 *   Array containing the target file's "real" path.
 *
 */
function hook_islandora_multi_importer_remote_file_get($url, $pid=NULL) {
  if ($url === '') { return FALSE; }
  $parsed_url = parse_url($url);

  // Assumes the file is somewhere in the public://imi_files path
  $basename = drupal_basename($parsed_url['path']);
  $pattern = "/$basename/";
  $options['key'] = 'filename';
  $file = file_scan_directory( "public://imi_files/", $pattern, $options );
  if (empty($file)) {
    $msg = "IMI was unable to find a file matching '$url' for ingest";
    if ($pid) {
      $msg .= "IMI was unable to find a file matching '$url' for ingest to object '$pid'.";
    } else {
      $msg .= ".";
    }
    drupal_set_message($msg, 'warning');
    watchdog('Islandora Multi Importer', $msg);
    return array();
  } else {
    $url = $file[$basename]->uri;
    return array(drupal_realpath($url));
  }
}

/**
 * hook_form_alter.
 *
 * Addresses https://github.com/mnylc/islandora_multi_importer/issues/38
 *
 * Note that this hook is implicitly, not explicitly, engaged by the
 *   islandora_multi_importer_form.  Intended use is to allow users of IMI to
 *   set custom default values for most input fields.
 *
 * Example values are from Digital Grinnell and the column order inherent in
 *   the Digital_Grinnell_MODS_Master Google Sheet, and its derivatives.
 *
 * @param $form
 * @param $form_state
 * @param $form_id
 *
 */
function hook_form_alter(&$form, &$form_state, $form_id) {

  // The big switch...
  switch($form_id) {

    // islandora_multi_importer_form
    case 'islandora_multi_importer_form':
      $form['cmodelmap']['basemapping']['rows']['cmodelmap_row']['cmodelmap']['#default_value'] = 4;        // cmodel - 4th column
      $form['objectmap']['objectmapping']['rows']['pidmap_row']['pidmap']['#default_value'] = 6;            // obj - 6th column
      $form['objectmap']['objectmapping']['rows']['parentmap_row']['parentmap']['#default_value'] = 3;      // parent - 3rd column
      $form['objectmap']['objectmapping']['rows']['labelmap_row']['labelmap']['#default_value'] = 7;        // title - 7th column
      $form['objectmap']['objectmapping']['rows']['sequencemap_row']['sequencemap']['#default_value'] = 5;  // sequence - 5th column
      $form['objectmap']['objectmapping']['rows']['dsmap_row']['dsremote']['#default_value'] = 'LOCAL';     // *local
      break;

  }

  return;
}