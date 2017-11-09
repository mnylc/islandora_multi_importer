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
