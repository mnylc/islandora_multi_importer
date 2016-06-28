<?php
/*
* Asynchronous client-to-server (DEALER to ROUTER)
*
* While this example runs in a single process, that is just to make
* it easier to start and stop the example. Each task has its own
* context and conceptually acts as a separate process.
* @author Ian Barber <ian(dot)barber(at)gmail(dot)com>
*/


/* ---------------------------------------------------------------------
* This is our client task
* It connects to the server, and then sends a request once per second
* It collects responses as they arrive, and it prints them out. We can
* run several client tasks in parallel, each with a different random ID.
*/
function islandora_multi_importer_client_task(array $object_data = array())
{
  for ($i = 10; $i < 15; $i++) {
    $object_data[] = 
    array(
      'type' => 'work',
      'id' => 'id'. $i,
      'data' => array(1,2,3,rand(0,100),5,6),
    );
  }

  $control_data = array(
    'type' => 'control',
    'data' => array('stop'),
  );

  $context = new ZMQContext();
  $client = new ZMQSocket($context, ZMQ::SOCKET_DEALER);

  // Generate printable identity for the client
  $identity = sprintf ("%04X", rand(0, 0x10000));
  $client->setSockOpt(ZMQ::SOCKOPT_IDENTITY, $identity);
  $client->connect("tcp://127.0.0.1:5570");

  $read = $write = array();
  $poll = new ZMQPoll();
  $poll->add($client, ZMQ::POLL_IN);

  $request_nbr = 0;

  $done_object_processing = array();
  $should_senddata = TRUE;
  $set_abort = FALSE;
  while ($should_senddata) {
    $events = $poll->poll($read, $write, 1000);
    $zmsg = new Zmsg($client);
    if ($events) {
      $zmsg->recv();
      //We get which object was processed
      $received_message = json_decode($zmsg->body(),true);
      if ($received_message['type'] == 'done') {
        $done_object_processing[] = $received_message['id'];
        error_log('client received from worker:'. $received_message['type']);
      }
      error_log(printf("%s: %s%s", $identity, $zmsg->body(), PHP_EOL));
      //Means all processing is done
      if (count($object_data) ==  count($done_object_processing)) {
        error_log('all processing done');
        $set_abort = TRUE;
      }
    }

    if (count($object_data)) {
      error_log('sending data');
      $zmsg = new Zmsg($client);
      $one_object_data = array_shift($object_data);
      $zmsg->body_set(json_encode($one_object_data))->send();
    }
    if ($set_abort) {
      $zmsg = new Zmsg($client);
      //$zmsg->body_set(json_encode($control_data))->send();
      $should_senddata = FALSE;
      error_log('Client exit');
    }
  }
}

/* ---------------------------------------------------------------------
* This is our server task
* It uses the multithreaded server model to deal requests out to a pool
* of workers and route replies back to clients. One worker can handle
* one request at a time but one client can talk to multiple workers at
* once.
*/
function islandora_multi_importer_server_task(IslandoraTuque $connection) {
  // Launch pool of worker threads, precise number should be based on specs
  // of your machine
  for ($thread_nbr = 0; $thread_nbr < 5; $thread_nbr++) {
    $pid = pcntl_fork();
    if ($pid == 0) {
      islandora_multi_importer_server_worker($connection);
      exit();
    }
  }

  $context = new ZMQContext();

  //  Frontend socket talks to clients over TCP
  $frontend = new ZMQSocket($context, ZMQ::SOCKET_ROUTER);
  $frontend->bind("tcp://127.0.0.1:5570");

  //  Backend socket talks to workers over ipc
  $backend = new ZMQSocket($context, ZMQ::SOCKET_DEALER);
  $backend->bind("ipc:///tmp/backend"); 

  //  Connect backend to frontend via a queue device
  //  We could do this:
  //  $device = new ZMQDevice($frontend, $backend);
  //  But doing it ourselves means we can debug this more easily

  $read = $write = array();
  //  Switch messages between frontend and backend
  while (true) {
    error_log('Server Running');
    $poll = new ZMQPoll();
    $poll->add($frontend, ZMQ::POLL_IN);
    $poll->add($backend, ZMQ::POLL_IN);

    $poll->poll($read, $write);
    foreach ($read as $socket) {
      $zmsg = new Zmsg($socket);
      $zmsg->recv();
      if ($socket === $frontend) {
        error_log("Request from client:");
        error_log($zmsg->__toString());
        $zmsg->set_socket($backend)->send();
      } elseif ($socket === $backend) {
        error_log("Request from worker:");
        error_log($zmsg->__toString());
        $zmsg->set_socket($frontend)->send();
      }
    }
  }
}

function islandora_multi_importer_server_worker(IslandoraTuque $connection) {
  module_load_include('inc', 'islandora_multi_importer', 'includes/utilities');
  error_log("Tuque connection alive?:".$connection->exists());
  $context = new ZMQContext();
  $worker = new ZMQSocket($context, ZMQ::SOCKET_DEALER);
  $worker->connect("ipc:///tmp/backend");
  $zmsg = new Zmsg($worker);

  $should_work = true;
  while ($should_work) {
    //  The DEALER socket gives us the address envelope and message
    $zmsg->recv();
    assert($zmsg->parts() == 2);
    $received_message = json_decode($zmsg->body(),true);
    if ($received_message['type'] == 'work') {
      error_log('processing'.$received_message['id']);
      usleep(rand(100000,300000) + 1);
      $object = $connection->repository->constructObject('test:'.$received_message['id']);
      $object->label = 'New Object '.$received_message['id'];
      $object->models = array("ir:citationCModel");
      $object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', "ir:citationCollection");
      $connection->repository->ingestObject($object);
      $response = array(
        'type' => 'done',
        'id' => 'id'. $received_message['id'],
      );
      $zmsg->body_set(json_encode($response));
      $zmsg->send(true);
    }
    if ($received_message['type'] == 'control') {
      error_log('exiting worker');
      $should_work = false;
    }
  }
  error_log('im out');
}

/* This main thread simply a server, and then
* waits for the server to finish.
*/
function islandora_multi_importer_zeroMQ_boot(IslandoraTuque $connection) {
  Database::closeConnection(); //To avoid forked processes to close the main one
  $pid = pcntl_fork();
  if ($pid == 0) {
    islandora_multi_importer_client_task();
    exit();
  }
  //Create a server only if not already running
  if (islandora_multi_importer_checkfree_socket("127.0.0.1", 5570)) {
    $pid = pcntl_fork();
    if ($pid == 0) {
      islandora_multi_importer_server_task($connection);
      exit();
    }
  }
  return;
}

/**
 * Check if process exists on Linux type OS
 * 
 * http://www.blrf.net/howto/25_PHP__How_to_check_if_PID_exists_on_Linux_.html
 *
 * @param int $pid Process ID
 * @param string $name Process name, null for no process name matching
 * @return bool
 */
function islandora_multi_importer_check_process($pid, $name = null) {
  // form the filename to search for
  $file = '/proc/' . (int)$pid . '/cmdline';
  $fp = false;
  if(file_exists($file))
    $fp = @fopen($file, 'r');
     // if file does not exist or cannot be opened, return false
  if(!$fp)
    return false;
  $buf = fgets($fp);
  // if we failed to read from file, return false
  if($buf === false){return false;}
  if($name !== null){
    // this code will also check if name matches
    $cmd = basename($buf);
    if(preg_match('/' . $name . '.*/', $cmd)){
      fclose($fp);
      return true;
    }else{
      // process was found, but name did not match
      fclose($fp);
      return false;
    }
  }else{
    // process found, name is null, return true
    fclose($fp);
    return true;
  }
}
/**
 * Check if socket port is in use
 * 
 * http://ca.php.net/manual/en/function.socket-connect.php#94350
 *
 * @param string $address Server IPV4 Address
 * @param int $port Socket Port
 * @return bool TRUE is port is free
 */ 
function islandora_multi_importer_checkfree_socket($address, $port) {
  if (isset($port) && ($socket=socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) &&
  (socket_connect($socket, $address, $port))) {
    socket_close($socket);
    return FALSE;
  }
  else {
   return TRUE;
  } 
 return FALSE;
}