/*
Description			: use 'net' module.
Default value		: /
The scope of value	: /
First used			: /
*/
var net = require('net');

/*
Description			: use 'RemoteCMD'.
Default value		: /
The scope of value	: /
First used			: /
*/
var remoteCmd = require('./node_modules/remote_cmd');

/*
Description			: use 'CProcess'.
Default value		: /
The scope of value	: /
First used			: /
*/
var cProcess = require('./node_modules/c_process');

/*
Description			: use 'OSInfo'.
Default value		: /
The scope of value	: /
First used			: /
*/
var osInfo = require('./node_modules/os_info');

/*
Description			: use 'fs'.
Default value		: /
The scope of value	: /
First used			: /
*/
var fs = require('fs');

/*
Description			: use 'http'.
Default value		: /
The scope of value	: /
First used			: /
*/
var http = require('http');

/*
Description			: use 'child_process'.
Default value		: /
The scope of value	: /
First used			: /
*/
var spawn = require('child_process').spawn;

/*
Description			: communication port with server.
Default value		: /
The scope of value	: /
First used			: /
*/
var PORT = new Array(8125,8124);

/*
Description			: communication ip with server.
Default value		: 223.4.21.219
The scope of value	: /
First used			: /
*/
var SER_IP = '223.4.21.219';

/*
Description			: save middleware id.
Default value		: 0000000000
The scope of value	: /
First used			: /
*/
var MIDWARE_ID = "0000000000";

/*
Description			: use date information.
Default value		: /
The scope of value	: /
First used			: /
*/
var date = new Date();

/*
Description			: for timer.
Default value		: /
The scope of value	: /
First used			: /
*/
var intervalObj;

/*******************************************
* start Bake_Tobacco_Monitor
********************************************/
cProcess.Run('Bake_Tobacco_Monitor', 1, -1, 0);

/*******************************************
* read version
********************************************/
fs.readFile('./conf/version', function(err, chunk){
	
	var ver = chunk.toString();
	
	ver = ver.substring(0, 3);
	
	console.log("run version: " + ver);
});	

/*******************************************
* read ser ip
********************************************/
fs.open("./conf/ser_ip", "r", function(err, fd){

	if (err)
	{
		throw err;
	}
	else
	{
		SER_IP = fs.readFileSync("./conf/ser_ip");
		
		if (0 !== SER_IP.length)
		{
			SER_IP = SER_IP.toString('ascii', 0, (SER_IP.length - 1));			
			console.log("service ip " + SER_IP);	
		}
		else
		{
			console.log("read ser ip failed!");
		}
	}

	fs.close(fd, function(){
	
		console.log("close ", fd);
	});
});

/*******************************************
* read mid id
********************************************/
fs.open("./conf/mid_id", "r", function(err, fd){
	
	if (err)
	{
		throw err;
	}
	else
	{
		MIDWARE_ID = fs.readFileSync("./conf/mid_id");
		
		if (0 !== MIDWARE_ID.length)
		{
			MIDWARE_ID = MIDWARE_ID.toString('ascii', 0, (MIDWARE_ID.length));
			MIDWARE_ID = MIDWARE_ID.substring(0, 10);
			
			console.log("MID_ID " + MIDWARE_ID);
			
			///*
			//--- send mid id to remote server. ---//
			intervalObj = setInterval(timeoutCallback, (60*1000));
			//*/					
		}
		else
		{
			console.log("read mid id failed!");
		}

		fs.close(fd, function(){
		
			console.log("close ", fd);
		});
	}
}); //--- end of fs.open("./conf/mid_id/mid_id", function(err, fd) ---//

function timeoutCallback()
{
	var clientSocket = new net.Socket();
	var cmdData = '';
			
	clearInterval(intervalObj);
	
	clientSocket.connect(PORT[0], SER_IP, function(){

		var json = 
		{
			type:3,
			address:MIDWARE_ID,
			data:[0,0,0,0,0,0,0,0,0]
		};
		var info;
						
		info = osInfo.getOSInfo();
		
		json.data[0] = info.totallMem;
		json.data[1] = info.usedMem;
		json.data[2] = info.cpusRate;
		json.data[3] = info.arch;
		json.data[4] = info.osPlatform;
		json.data[5] = info.cpuModel;
		json.data[6] = info.uptime;
		
		console.log(date + " CONNECTED:" + SER_IP + ":" + PORT[1]);
		console.log("UPLOAD MID ID", MIDWARE_ID);
		
		fs.readFile('/tmp/startup', function(err, chunk){

			var hwc = spawn("hwclock", []);
			var startup = "null0";
			var curHwc;
			
			if (err)
			{
				startup = "null0";
			}
			else
			{
				startup = chunk.toString();
			}
			
			startup = startup.substring(0, (startup.length - 1));

			json.data[7] = startup;

			hwc.stdout.once('data', function(chunk){

				curHwc = chunk.toString();
				curHwc = curHwc.substring(0, (curHwc.length - 1));

				json.data[8] = curHwc;
			});

			hwc.once('close', function(err){

				if (err)
				{
					return console.error(err);
				}
				else
				{
					clientSocket.write(JSON.stringify(json));			
				}
			});
		});
						
	}); //--- end of clientSocket.connect(PORT[1], SER_IP, function() ---//
	
	clientSocket.on('data', function(chunk){
	
		cmdData += chunk;
	});
	
	clientSocket.on('end', function(){
	
		var jsons = [];

		console.log("cmd data:" + cmdData);

		try
		{
			if (cmdData.length)
			{
				var cmdJsons = JSON.parse(cmdData);
			
				if (cmdJsons.length)
				{								
					for (var i = 0; i < cmdJsons.length; ++i)
					{
						var jsonElement = 
						{
							type:cmdJsons[i].command.type,
							address:cmdJsons[i].command.address,
							data:cmdJsons[i].command.data
						};
						
						jsons.push(jsonElement);
					}
							
					if (MIDWARE_ID === jsons[0].address)
					{
						remoteCmd.proRemoteCmd(PORT[1], SER_IP, jsons);
					}
					else
					{
						console.log("mid id error!");
					}
				}
			}		
		}
		catch(err)
		{
			console.log(err);
		}	
		
		clientSocket.destroy();	
	});

	clientSocket.on('error', function(err){
		
		console.log("ERROR:", err.errno);
		
		clientSocket.destroy();
	});
	
	clientSocket.on('close', function(){
	
		console.log("client close");
		
		clientSocket.removeAllListeners();
		
		intervalObj = setInterval(timeoutCallback, (60*1000));
		
	});	
}
 
/*******************************************
* http server
********************************************/
http.createServer(function(req, res){
		
	var body = "";
	
	console.log(req.method + " " + "http://" + req.headers.host + req.url);
	
	req.on('data',function(chunk){
		
		body += chunk;
	});
	
	req.on('end', function(){
		
		proHttpRequest(req, body, res);
		
		req.removeAllListeners();
	});
		
}).listen(8080);

function proHttpRequest(req, body, response)
{
	if ("GET" === req.method || "get" === req.method)
	{
		proGet(req.url, response);
	}
	else if ("POST" === req.method || "post" === req.method)
	{
		proPost(req.url, body, response);
	}
}

function proGet(url, response)
{
	if ('/' === url)
	{
		sendHttpResponse_HTML(response, "index.html");	
	}
	else if ('/favicon.ico' === url)  
	{
		sendHttpResponse_ICON(response);
	}
	else if ("/mid_id" === url)
	{
		var midId = "";
		
		fs.readFile('./conf/mid_id', function(err, chunk){
			
			midId = chunk.toString();
			
			fs.readFile('./conf/mid_id_ch', function(err, chunk){
				
				if (err)
				{
					midId = midId + "-----------------------"; 
				}
				else
				{
					midId = midId + chunk.toString();
				}

				sendHttpResponse_TEXT(response, midId, 200);
			});			
		});		
	}
	else if ("/cur_slave_addrs_00" === url)
	{
		fs.readFile('./conf/slaves_addr/aisle_00', function(err, chunk){
		
			var sum = 0;
			var tmp;
			
			tmp = chunk.toString();
			sum = tmp.length / 6;
			tmp = tmp + "</br></br>" + "当前自控仪数量：" + sum;

			sendHttpResponse_TEXT(response, tmp, 200);
		});			
	}
	else if ("/version" === url)
	{
		fs.readFile('./conf/version', function(err, chunk){
			
			sendHttpResponse_TEXT(response, chunk, 200);
		});			
	}
	else if ("/tmp_log" === url)
	{
		fs.readFile('/tmp/tmp.log', function(err, chunk){
			
			if (err)
			{
				sendHttpResponse_TEXT(response, "{tmp.log}</br>", 200);
			}
			else
			{
				sendHttpResponse_TEXT(response, chunk, 200);
			}
		});			
	}
}

function proPost(url, body, response)
{
	if ("/slave_addrs" === url)
	{	
		var jsons = [
		{
			type:15,
			address:MIDWARE_ID,
			data:[]
		}];
		var bodyJson;
		var id = 0;
		var i = 0;
		
		bodyJson = JSON.parse(body);
		
		if (0 < bodyJson.addresses.length)
		{
			
			jsons[0].data.push(bodyJson.action);
			jsons[0].data.push(bodyJson.addresses.length);
			
			for (i = 0; i < bodyJson.addresses.length; ++i)
			{	
				id = bodyJson.addresses[i];	
				jsons[0].data.push((id >> 8));
				jsons[0].data.push((id & 0x00ff));
			}
			
			jsons[0].data.push(Number(bodyJson.aisle));
		
			console.log(jsons);
		
			//----------------------------------------------------------------------//
			remoteCmd.proRemoteCmd(PORT[1], SER_IP, jsons);
			//----------------------------------------------------------------------//
	
			sendHttpResponse_TEXT(response, " ", 201);
		} //-- if (0 < bodyJson.addresses.length) --//
	}
	else if ("/mid_id" === url)
	{		
		var json;
		
		json = JSON.parse(body);
		console.log(json);
		if ("" !== json.midId)
		{			
			fs.writeFile('./conf/mid_id', json.midId, function(err){
				
				fs.writeFile('./conf/mid_id_ch', json.midIdCh, function(err){
				
					sendHttpResponse_TEXT(response, json.midId, 201);
				});			
			});			
		}		
	}	
}

function sendHttpResponse_HTML(response, html)
{
	fs.readFile('./web/' + html, function(err, chunk){

		response.setHeader('Content-Type', 'text/html');
		response.setHeader("Cache-Control", "no-cache");
		response.writeHeader(200);
		response.write(chunk);
		response.end();
	});
}

function sendHttpResponse_TEXT(response, text, status)
{
	response.setHeader('Content-Type', 'application/string');
	response.setHeader("Cache-Control", "no-cache");
	response.writeHeader(status);
	response.write(text);
	response.end();	
}

function sendHttpResponse_ICON(response)
{
	fs.readFile('./web/favicon.ico', function(err, chunk){

		response.writeHeader(200);
		response.write(chunk);
		response.end();
	});		
}









