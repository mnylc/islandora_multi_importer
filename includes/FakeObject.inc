<?php

/**
 * @file
 * Provides a fake, empty, Abstract Fedora Object.
 * @author Diego Pino Navarro
 */

/**
 * FakeObject is a do nothing tuque/AbstractObject derived class.
 */
class FakeObject extends AbstractObject {

  /**
   * {@inheritdoc}
   */
  public function delete() {
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function getDatastream($id) {
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function purgeDatastream($id) {
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function constructDatastream($id, $control_group = 'M') {
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function ingestDatastream(&$ds) {
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function offsetExists($offset) {

  }

  /**
   * {@inheritdoc}
   */
  public function offsetSet($offset, $value) {

  }

  /**
   * {@inheritdoc}
   */
  public function offsetGet($offset) {

  }

  /**
   * {@inheritdoc}
   */
  public function offsetUnset($offset) {

  }

  /**
   * {@inheritdoc}
   */
  public function count() {
    return 0;
  }

  /**
   * {@inheritdoc}
   */
  public function getIterator() {

  }

}
