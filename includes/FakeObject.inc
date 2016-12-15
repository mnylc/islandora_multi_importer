<?php

/**
* @file
* Provides a fake, empty object to be used as a stub where needed
*/

class FakeObject extends AbstractObject {


  public function delete() {return false;}

  /**
  * Get a datastream from the object.
  *
  * @param string $id
  *   The id of the datastream to retreve.
  *
  * @return AbstractDatastream
  *   Returns FALSE if the datastream could not be found. Otherwise it return
  *   an instantiated Datastream object.
  */
  public function getDatastream($id) { return FALSE;}

  /**
  * Purges a datastream.
  *
  * @param string $id
  *   The id of the datastream to purge.
  *
  * @return boolean
  *   TRUE on success. FALSE on failure.
  */
  public function purgeDatastream($id) { return FALSE;}

  /**
  * Factory to create new datastream objects. Creates a new datastream object,
  * this object is not ingested into the repository until you call
  * ingestDatastream.
  *
  * @param string $id
  *   The identifier of the new datastream.
  * @param string $control_group
  *   The control group the new datastream will be created in.
  *
  * @return AbstractDatastream
  *   Returns an instantiated Datastream object.
  */
  public function constructDatastream($id, $control_group = 'M') { return FALSE;}

  /**
  * Ingests a datastream object into the repository.
  */
  public function ingestDatastream(&$ds) { return FALSE;}
  
  public function offsetExists($offset) { 

  } 

  public function offsetSet($offset, $value) { 

  } 

  public function offsetGet($offset) { 

  } 

  public function offsetUnset($offset) { 

  } 
  
  public function count() {
    return 0;
  }
  
  public function getIterator() {
    
  }
}
