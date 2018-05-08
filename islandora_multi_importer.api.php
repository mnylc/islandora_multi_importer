<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * hook_islandora_multi_importer_remote_file_get example from https://digital.grinnell.edu and
 *   https://dgadmin.grinnell.edu
 *
 * @param $url
 *   Path to the file to be fetched.  Default behavior strips the 'basename' from $url and looks
 *   for that file somewhere in $target.
 * @return array
 *   On error or failure an empty array is returned.
 *   On success, a single array element LOCAL path like /mnt/storage/exmample.xml
 *
 * Important!  This function requires that your target directory of files be mounted to
 *   the host server as $target (see below).  The mount statement should look something
 *   like this:
 *
 *  sudo mount -t cifs -o username=mcfatem //storage.grinnell.edu/LIBRARY/mcfatem/PHPP_Content /mnt/storage
 *
 */
function hook_islandora_multi_importer_remote_file_get($url) {
  if ($url === '') { return array(); }  // return an empty array if $url is blank
  $parsed_url = parse_url($url);

  // Assumes the file is somewhere in the /mnt/storage path
  $target = "/mnt/storage";

  $basename = drupal_basename($parsed_url['path']);
  $pattern = "/" . preg_quote($basename) . "/";
  $options['key'] = 'filename';
  $file = file_scan_directory($target, $pattern, $options);

  $msg = "IMI hook " . __FUNCTION__ . " was invoked ";

  // Nothing found...report and return an empty array.

  if (empty($file)) {
    $msg .= "and was unable to find a file matching '$url' in '$target' for ingest. ";
    $msg .= "Make sure that you have mounted your files as '$target', and that they can be read by 'www-data'!";
    drupal_set_message($msg, 'warning');
    watchdog('Islandora Multi Importer', $msg);
    return array();
  }

  // File found!  Report and send back the URI in an array.

  else {
    $uri = $file[$basename]->uri;
    $msg .= "and found file '$uri' in '$target' for ingest.";
    drupal_set_message($msg, 'status');
    return array($uri);
  }
}



