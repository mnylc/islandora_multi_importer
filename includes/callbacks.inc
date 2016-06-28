<?php

/**
 * @file
 * includes/callbacks.inc
 * 
 * Contains Ajax Callbacks  
 * @author Diego Pino Navarro
 */



function islandora_multi_importer_ajax_updaterecordpreview(&$form, &$form_state) {
    $selector ='#islandora-multi-rows-preview-replace';
    $commands = array();
    $commands[] = ajax_command_replace('#islandora-multi-rows-preview-replace',render($form['step3']['preview_data']));
    return array('#type' => 'ajax', '#commands' => $commands);
}
