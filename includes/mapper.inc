<?php
/**
 * @file
 * includes/mapper.inc
 * 
 * Contains mapping utilities 
 * @author Diego Pino Navarro
 */

/**
 * Validates CMODELs from array.
 *
 * @param array $cmodels
 *   An array with CMODELS
 *
 * @return array
 *   array of associative arrays containing unique CMODELS and their DS
 */
function islandora_multi_importer_mapper_check_cmodels($cmodels) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $cmodelsWithDs = array();
  $cmodelsTmp = array();
  if (count($cmodels)) {
    $cmodelsTmp = array_map('trim', $cmodels);
    $cmodelsTmp = array_unique($cmodelsTmp, SORT_STRING);
    $existingCmodels = islandora_get_content_models();
    //Gets rid of label, makes flat
    $existingCmodels2 = array_keys($existingCmodels);
    //Check which ones of our unique input are valids
    $validCmodels = array_intersect($cmodelsTmp, $existingCmodels2);
    foreach($validCmodels as $cmodel) {
      $cmodelsWithDs[$cmodel] = islandora_get_datastreams_requirements_from_models(array($cmodel));
    }
  }
  return $cmodelsWithDs;
}

/**
 * Get the list of derivatives for a CMODEL
 *
 * @param String $cmodel
 *   The CMODEL for which we want the Derivatives
 * @param &array $options
 *   Options for derivatives, will be updated to default force to FALSE.
 */
function islandora_multi_importer_mapper_get_derivatives($cmodel) {
    module_load_include('inc', 'islandora', 'includes/utilities');
   /* $options
    *   An array of parameters containing:
    *   - force: Bool denoting whether we are forcing the generation of
    *     derivatives.
    *   - source_dsid: (Optional) String of the datastream id we are generating
    *     from or NULL if it's the object itself.
    *   - destination_dsid: (Optional) String of the datastream id that is being
    *     created. To be used in the UI.
   
    );*/
   
   $options = array(
     'force' => FALSE,
     'source_dsid' => NULL,
     'destination_dsid' => NULL,
   );
    $derivatives = islandora_invoke_hook_list(
      ISLANDORA_DERIVATIVE_CREATION_HOOK,
      array($cmodel),
      array()
    );
    foreach (islandora_build_hook_list(ISLANDORA_DERIVATIVE_CREATION_HOOK, array($cmodel)) as $hook) {
      drupal_alter($hook, $derivatives);
    }
    uasort($derivatives, 'drupal_sort_weight');
    return $derivatives;
}


/**
 * Filter the derivative functions to only call those which are valid.
 *
 * @param array $hooks
 *   An array of hooks to be filtered depending on options.
 * @param array $options
 *   An array of options for the derivative generation.
 * @return array
 *   Returns the filtered array of hooks to be ran.
 */
function islandora_multi_importer_mapper_filter_derivatives($hooks, $options) {
  if (array_key_exists('source_dsid', $options)) {
    $hooks = array_filter($hooks, function ($filter_hook) use ($options) {
      return array_key_exists('source_dsid', $filter_hook) &&
      $filter_hook['source_dsid'] == $options['source_dsid'];
    });
  }
  if (array_key_exists('destination_dsid', $options)) {
    $hooks = array_filter($hooks, function ($filter_hook) use ($options) {
      return array_key_exists('destination_dsid', $filter_hook) &&
      $filter_hook['destination_dsid'] == $options['destination_dsid'];
    });
  }
  //Since we are working on DS-COMPOSITE here, we don't have an object yet. We just need to know
  //Which derivatives are available for a given CMODEL
  return $hooks;
}