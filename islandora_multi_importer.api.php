<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * Implements hook_islandora_multi_importer_remote_file_get for https://digital.grinnell.edu.
 *
 * @param $url
 *   Path to the file to be fetched.  Behavior of this hook depends on the transfer scheme specified.
 *     Default behavior strips the 'basename' from $url and looks for that file somewhere in public://imi_files.
 *     @TODO... if smb:// is prepended to the $url implement a Samba file fetch here.
 * @return array
 *   On error or failure an empty array is returned.
 *   On success, a single array element LOCAL path like /var/www/drupal7/sites/default/files/imi_files/exmample.xml
 */
function hook_islandora_multi_importer_remote_file_get($url) {
  if ($url === '') {
    return array();   // return an empty array
  }

  $parsed_url = parse_url($url);

  // Assumes the file is somewhere in the public://imi_files path

  $basename = drupal_basename($parsed_url['path']);
  $pattern = "/" . preg_quote($basename) . "/";
  $options['key'] = 'filename';
  $file = file_scan_directory("public://imi_files", $pattern, $options);

  $msg = "IMI hook " . __FUNCTION__ . " was invoked ";

  // Nothing found...report and return an empty array.

  if (empty($file)) {
    $msg .= "and was unable to find a file matching '$url' for ingest.";
    drupal_set_message($msg, 'warning');
    watchdog('Islandora Multi Importer', $msg);
    return array();
  }

  // File found!  Report and send back the URI in an array.

  else {
    $uri = $file[$basename]->uri;
    $msg .= "and found file '$uri' for ingest.";
    drupal_set_message($msg, 'status');
    return array($uri);
  }
}
