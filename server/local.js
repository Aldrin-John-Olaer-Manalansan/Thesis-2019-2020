const _smart_home_serial_key_ = 'amFsZSc';
const BroadcastType = 'Local';
var BroadcastPort = 417;



const path = require('path')
const fs = require('fs')
const express = require('express')
const https = require('https')
const WebSocket = require('ws');
const url = require('url');
const EventEmitter = require('events');
const convert = require('base64-js')

class MyEmitter extends EventEmitter {}

const trigger = new MyEmitter();

var certs = {
  //key: fs.readFileSync(path.resolve('ssl/server/my-server.key.pem')),
  key: fs.readFileSync(path.resolve('certs/server.key.pem')),

  //cert: fs.readFileSync(path.resolve('ssl/server/my-server.crt.pem')),
  cert: fs.readFileSync(path.resolve('certs/server.crt.pem')),


  //ca: fs.readFileSync(path.resolve('ssl/server/my-private-root-ca.crt.pem'))
};


if(BroadcastType=='Local'){
  var certificates=true;
  BroadcastPort = 417;
}
else {
  var certificates=false;
  BroadcastPort= 3527;
}



if(BroadcastType=='Local'){
  require('ssl-root-cas').inject().addFile(path.resolve('ssl/server/my-private-root-ca.crt.pem'));


  front = require('http').createServer(certs, function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write("hello world\n")
    res.end();
  }).listen(6969);
  console.log('Local broadcasting');
}







var certificates=false;


server = https.createServer( (certificates||0?certs:{}));
console.log('Online');




var FrameHolder = '';
var Cache = '';
const wss = new WebSocket.Server({ server:server });

wss.on('connection', function connection(ws) {
  var auth = false;
  var token;
  ws.send(JSON.stringify({type:'handshake', status:'inactive'}));
  // message from client
  ws.on('message', function incoming(message) {
    // Check if user follows standard
    console.log('received: %s', JSON.stringify(message));
    try{
    var user_request = JSON.parse(message);
    }
    catch(e){
      ws.close()
      console.log('A user has been rejected');
      return;
    }
    //Handshake
    if(auth==false  ){
      //check validity
      if(!user_request.hasOwnProperty('key') &&!user_request.hasOwnProperty('type')){
        ws.close();
        console.log('User does not comply');
      }
      if(user_request.key==_smart_home_serial_key_ && user_request.type=='serial' ){
        //Generate stamp
        var user_token = Date.now();
        token =(user_token);
        ws.send(JSON.stringify({ type:'acknowledge', status:BroadcastType, token:user_token  }));
        console.log('User has been acknowledged. Sending token: '+ user_token);
        Home.camera.map((cctv)=>{getframe(cctv.serial);})
        auth=true;
        return;
      }
      else{
        console.log('User has the wrong key');
        ws.close();}
        return;
     }
    //Check validity
    if(user_request.hasOwnProperty('status') && user_request.hasOwnProperty('type')){
      if(user_request.status==token){ ws.send(handle(user_request)); return; }
      console.log('Token does not match!');
    }
  });
  // Active messaging where the server sends data automatically when file change occurs

  /*fs.watch(path.resolve('bridge/userdata.json'), {
    persistent: true
  }, function(event, filename) {
    if(auth==false) return;
    Home = JSON.parse(fs.readFileSync(path.resolve('bridge/userdata.json')))
    ws.send(JSON.stringify({ type:'notify', status:'update' }))

  });
*/
  trigger.on('update', function(){
    if(auth==false) return;
    //Home = JSON.parse(fs.readFileSync(path.resolve('bridge/userdata.json')))
    //console.log('Sending Home data');
    if( Home.notification !='none'){ ws.send(JSON.stringify({ type:'notification', status:Home.notification })); console.log('Socket not Found!');}
    ws.send(JSON.stringify({ type:'data', status:getData()}))
  });

  trigger.on('push',function(){
    console.log('Sending Frame...');

    ws.send(JSON.stringify(FrameHolder));
  })
});








server.listen(BroadcastPort);

if(BroadcastType=='Local'){
  fs.watch(path.resolve('bridge/userdata.json'), { persistent: true},
  function(event, filename) {
    setTimeout(function(){
      var bridge = JSON.parse(fs.readFileSync(path.resolve('bridge/queue.json')));
      Home = readFile();
      var timestamp = Home.timestamp;


      console.log('timestamp: ' + timestamp);
      bridge = bridge.filter(request =>{ return request.timestamp > timestamp });

      console.log('Queue: '+JSON.stringify(bridge));
      fs.writeFileSync(path.resolve('bridge/queue.json'), JSON.stringify(bridge));
      trigger.emit('update');
      console.log(event + " event occurred on " + filename); /* Debug */
    },100);

  });
}



//// Methods

const readFile = function(){
  try {
    return JSON.parse(fs.readFileSync(path.resolve('bridge/userdata.json')))
  } catch (e) {
    return readFile()
  }
};

// Home OBJECT
var Home = readFile();



const check_integrity = function(sample, reference){
  var integrity=true
  received_data = Object.keys(sample);
  reference.map( information =>{
    if(!received_data.includes(information)) {console.log('Corrupted data. Missing: '+information); integrity=false;}
  })
  return integrity;
};





const handle = function(user_request){
  // Validate message if json

  // data init
  data = { type:'reply', status:'placeholder' };
  // check for contents


  //to do listen//check for serial key
  //switch responses depending on request
  //return desired object

  // User request are either commands or fetch

  //fetch ------ register ------- remove -------
  console.log('User has requested: ' + user_request.type );
  switch (user_request.type) {
    // DATA Functions
    case 'fetch':
      data.type = 'data'
      data.status = getData();
      console.log('Sending home data');
      console.log(data.status);
      break;
      // MASTER FUNCTIONS ------------------------------------------------------
    case 'register': // register appliance to masterlist
      //open user object
      if  (check_integrity(user_request,['serial'])){
        queue('register',user_request.serial)
      }

      break;
    case 'remove': //  remove appliance from masterlist
      //open user object
      if  (check_integrity(user_request,['serial','item'])){
      queue('remove',user_request.serial, 0 ,user_request.item)}

      break;

    // Automation
    case 'automation': // Switch on or off
      if  (check_integrity(user_request,['serial','socket','automation_enabled','automation'])){
        queue(
          'automation',
          user_request.serial,
          user_request.socket,
          {
            enable:user_request.automation_enabled,
            time:user_request.automation
          }
        );
      }
      break;
      // BASIC / COMMON Functions ----------------------------------------------
    case 'toggle':
      if (check_integrity(user_request,['serial','socket','state']))
      {
        queue('toggle', user_request.serial, user_request.socket, user_request.state );
        console.log('Toggle complete');
      }
      break;

    case 'rename':
      if  (check_integrity(user_request,['serial','socket','name','device']))
        {
          queue('rename', user_request.serial, user_request.socket, {name:user_request.name, device:user_request.device });
          console.log('rename complete');
          }

      break;
      // CCTV FUNCTIONS ------------------------------------------------------
    case 'frame':
      if  (check_integrity(user_request,['serial']))
        {
           getframe(user_request.serial);
          //console.log('Frame: '+ frame);
        }
      //details
      //data.status =

      break;
    case 'feed':
    break;
    default:
    data.status = 'No response for: ' + user_request.type;
  }
  // Data
  // DATA
  //console.log('Sending: ' + JSON.stringify(data));
  return JSON.stringify(data);
}
const queue = function(request, serial='', socket=1, data=0){
  data = {request:request, serial:Array.from(convert.toByteArray(serial+'=')), socket:socket, data:data, timestamp:Date.now()};
  cache = JSON.parse(fs.readFileSync(path.resolve('bridge/queue.json'))),
  cache.push(data);
  console.log(cache);
  fs.writeFileSync(path.resolve('bridge/queue.json'), JSON.stringify(cache));
}


function getData() {
  try {
    var userdata = JSON.parse(fs.readFileSync(path.resolve('bridge/userdata.json')));
  } catch (e) {
    console.log('Userdata is corrupted: ' +e );
    return 'No data';
  }
  //console.log(JSON.stringify(userdata));
  //Prepare Data
  //Get all possible addresses
  var logs=[];
  var consumption = {};
  userdata.appliance.map( sample =>{
    sample.serial = convert.fromByteArray(sample.serial).replace('=','')
    try {
      logs = logs.concat(Object.keys(sample.consumption));
    } catch (e) {
      console.log(Object.keys(sample.consumption)+ sample.name);
    }
  });
  logs.sort();
  logs = [...new Set(logs)];
  //Loop through addresses to all appliance
  // Add Their values to form a total
  //Append values to home total
  //Repeat for home data
  logs.map( log=>{
    //Check if home has the object
    consumption[log]={data:[0], total:0};
    userdata.appliance.map( appliance=>{
      var total = 0;
      try {
        //appliance.consumption[log]['total']=total;
        appliance.consumption[log]['total']=total;
        appliance.consumption[log].data.map((val,i)=>{
          val = Math.round(val * 100) / 100;
          if(consumption[log].data[i]==null){
            consumption[log].data[i]=val;
            total+=val;
          }
          else{
            consumption[log].data[i]+=val;
            total+=val;
          }
          total = Math.round(total * 100) / 100;
          consumption[log].data[i]= Math.round(consumption[log].data[i] * 100) / 100
          //appliance.consumption[log]['total']+=total.toFixed(2);
        });

        consumption[log]['total']+=Math.round(total * 100) / 100;

        appliance.consumption[log]['total']=total || 0;
        appliance.consumption[log]['total']+=0;
      } catch (e) { /* Skip */}

    });
  });
  logs.map( log=>{
    var total=0;
    consumption[log].data.map( val=>{
      total+=val;
    });
    consumption[log]['total']=Math.round(total * 100) / 100;
  })
  userdata['consumption']=consumption;

  userdata.camera.map( sample =>{
    sample.serial = convert.fromByteArray(sample.serial).replace('=','')
  });


  // DATA LOG
  // DATA LOG
  //console.log(JSON.stringify(userdata));
  return userdata;
};


const getframe = function(){

  Home.camera.map(val=>{
      console.log('Iteration: '+ val.serial);
      //var frame ='No data';
      try {
        var init = false;
        var tunnel = new WebSocket(val.socket);
        tunnel.onopen=()=>{ console.log('Tunnel open');}
        tunnel.onmessage = e=>{
          if(init==true)return;
          init = true;

          FrameHolder = {type:'snap', serial:convert.fromByteArray(val.serial).replace('=',''), status:convert.fromByteArray(e.data)}
          //console.log('Sending Frame: '+FrameHolder);
          console.log('push');
          trigger.emit('push');
          /*wss.on('connection', function connection(ws){
            console.log(e.data.length)
            if(init)return;
            init = true;
            console.log('Frame: sent');
            data = bytesToBase64(e.data)
            FrameHolder = data;
            trigger.emit('push')
            //ws.send(JSON.stringify({ serial:serial, status:data, type:'snap'}));
            tunnel.close();
          })*/
          }
        tunnel.onerror=e=>{ tunnel.close()}

      }
      catch (e) {console.log('Failed to load socket: '+e);}

     //return frame;
  });
}






/*


const base64abc = (() => {
 let abc = [],
   A = "A".charCodeAt(0),
   a = "a".charCodeAt(0),
   n = "0".charCodeAt(0);
 for (let i = 0; i < 26; ++i) {
   abc.push(String.fromCharCode(A + i));
 }
 for (let i = 0; i < 26; ++i) {
   abc.push(String.fromCharCode(a + i));
 }
 for (let i = 0; i < 10; ++i) {
   abc.push(String.fromCharCode(n + i));
 }
 abc.push("+");
 abc.push("/");
 return abc;
})();

function bytesToBase64(bytes) {
 let result = '', i, l = bytes.length;
 for (i = 2; i < l; i += 3) {
   result += base64abc[bytes[i - 2] >> 2];
   result += base64abc[((bytes[i - 2] & 0x03) << 4) | (bytes[i - 1] >> 4)];
   result += base64abc[((bytes[i - 1] & 0x0F) << 2) | (bytes[i] >> 6)];
   result += base64abc[bytes[i] & 0x3F];
 }
 if (i === l + 1) { // 1 octet missing
   result += base64abc[bytes[i - 2] >> 2];
   result += base64abc[(bytes[i - 2] & 0x03) << 4];
   result += "==";
 }
 if (i === l) { // 2 octets missing
   result += base64abc[bytes[i - 2] >> 2];
   result += base64abc[((bytes[i - 2] & 0x03) << 4) | (bytes[i - 1] >> 4)];
   result += base64abc[(bytes[i - 1] & 0x0F) << 2];
   result += "=";
 }
 return result;
}

*/


as = convert.fromByteArray([162,111,4,54,92])
ds = convert.fromByteArray([76,224,23,108,172])
console.log(as);
console.log(ds);
//console.log(atob(bytesToBase64([76,224,23,108,172])));
//console.log(atob(bytesToBase64([162,111,4,54,92])));
