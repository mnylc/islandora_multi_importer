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
   * This is an Fake object for Mocking relationships related to this object.
   *
   * @var FakeRelsExt
   */
  public $relationships;

  /**
   * {@inheritdoc}
   */
  public $models;

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

  /**
   * Constructogecko.
   *
   * @param string $id
   *   A fakeobject fakeid.
   * @param array $models
   *   An list of real Islandora CMODELS.
   */
  public function __construct($id = NULL, array $models = array()) {
    parent::__construct();
    $this->id = $id;
    $this->relationships = new FakeRelsExt($models);
  }

}

/**
 * FakeRelsExt is a do nothing RelsClass.
 */
class FakeRelsExt {

  const FAKE_MODEL_URI = 'info:fedora/fedora-system:def/model#';
  /**
   * An array of strings containing the content models of the object.
   *
   * @var array
   */
  public $models;

  /**
   * Fake add, method does nothing.
   */
  public function add($predicate_uri, $predicate, $object, $type = RELS_TYPE_URI) {
  }

  /**
   * Fake get, only acts on stored ->models.
   */
  public function get($predicate_uri = NULL, $predicate = NULL, $object = NULL, $type = RELS_TYPE_URI) {
    // Since we are Fake we will deal only with CMODELS.
    if (!isset($this->models) || empty($this->models)) {
      return array();
    }
    if (($predicate_uri == $this::FAKE_MODEL_URI) && ($predicate == 'hasModel')) {
      $results = array();
      if (isset($object) && in_array($object, $this->models)) {
        $themodels = array($object);
      }
      else {
        $themodels = $this->models;
      }
      foreach ($themodels as $model) {
        $result = array();
        $result['predicate'] = array();
        $result['predicate']['value'] = $predicate;
        $result['predicate']['alias'] = 'fedora-model';
        $result['predicate']['namespace'] = "info:fedora/fedora-system:def/model#";
        $object = array();

        $object['literal'] = FALSE;
        $object['value'] = $model;
        $result['object'] = $object;
        $results[] = $result;
      }
      return $results;
    }

    return array();
  }

  /**
   * Constructolizzard.
   *
   * @param array $models
   *   An list of real Islandora CMODELS.
   */
  public function __construct(array $models = array()) {
    $this->models = $models;
  }

}
