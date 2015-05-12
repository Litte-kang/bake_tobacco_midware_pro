/*
Description			: use 'net' module.
Default value		: /
The scope of value	: /
First used			: /
*/
var net = require('net');

/*
Description			: use 'remote_cmd'.
Default value		: /
The scope of value	: /
First used			: /
*/
var remoteCmd = require('./node_modules/remote_cmd');

/*
Description			: use 'c_process'.
Default value		: /
The scope of value	: /
First used			: /
*/
var cProcess = require('./node_modules/c_process');

/*
Description			: use 'os_info'.
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

/*
Description			: we upload middleware base information when counter00 is 360.
Default value		: /
The scope of value	: /
First used			: /
*/
var counter00 = 0;

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
			intervalObj = setInterval(timeoutCallback, (1000));
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
	
	//counter00++;
		
	clearInterval(intervalObj);
	
	clientSocket.setTimeout(1000, function(){

		console.log("connect:" + SER_IP + ":" + PORT[0] + " timeout");

		clientSocket.destroy();

	});

	clientSocket.connect(PORT[0], SER_IP, function(){

		var json = 
		{
			type:3,
			address:MIDWARE_ID,
			data:[0,0,0,0,0,0,0,0,0]
		};
		var info;
		
		if (0 == counter00)
		{		
			counter00 = 0;
				
			info = osInfo.getOSInfo();
		
			json.data[0] = info.totallMem;
			json.data[1] = info.usedMem;
			json.data[2] = info.cpusRate;
			json.data[3] = info.arch;
			json.data[4] = info.osPlatform;
			json.data[5] = info.cpuModel;
			json.data[6] = info.uptime;
				
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
			}); //--- end of fs.readFile('/tmp/startup', function(err, chunk) ---//
		} //--- end of if (0 == counter00) ---//
						
	}); //--- end of clientSocket.connect(PORT[1], SER_IP, function() ---//
	
	clientSocket.on('data', function(chunk){
	
		cmdData += chunk;
	});
	
	clientSocket.on('end', function(){
	
		var jsons = [];

		console.log("cmd data:" + cmdData);

		if (cmdData.length)
		{
			var cmdJsons;
			
			try
			{
				cmdJsons = JSON.parse(cmdData);
			}
			catch (err)
			{
				console.log("json parse error");
				cmdJsons = '';
			}
			
			if (cmdJsons.length)
			{								
				for (var i = 0; i < cmdJsons.length; ++i)
				{					
					jsons.push(cmdJsons[i]);
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
		
		clientSocket.destroy();	
	});

	clientSocket.on('error', function(err){
		
		console.log("ERROR:", err.errno);
		
		clientSocket.destroy();
	});
	
	clientSocket.on('close', function(){
	
		console.log("client close");
		
		clientSocket.removeAllListeners();
		
		intervalObj = setInterval(timeoutCallback, (1000));
		
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
	else if ("/slave_data" == url)
	{
		var jsons = [{
			type:17,
			address:MIDWARE_ID,
			data:[0,0]
		}];
		
		remoteCmd.proRemoteCmd(PORT[1], SER_IP, jsons);
		
		sendHttpResponse_TEXT(response, "OK", 200);
	}
}

function proPost(url, body, response)
{
	if ("/slave_addrs" === url)
	{	
		var bodyJson;
		var id = 0;
		var i = 0;
		
		bodyJson = JSON.parse(body);
		
		if (0 < bodyJson.addresses.length)
		{
			fs.readFile('./conf/slaves_addr/aisle_00', function(err, chunk){
		
				var sum = 0;
				var tmp = "";
				var slavesAddr = '';
	
				tmp = chunk.toString();
	
				sum = tmp.length / 6;
				
				if (1 === bodyJson.action)	//-- add --//
				{
					for (i = 0; i < bodyJson.addresses.length; ++i)
					{	
						id = "00000" + bodyJson.addresses[i];	
						id = id.substring((id.length-5)) + "\n";
						
						tmp += id;
					}	
					
					slavesAddr = tmp;			
				}
				else if (0 === bodyJson.action) //-- inc --//
				{
					id = "00000" + bodyJson.addresses[0];	
					id = id.substring((id.length-5)) + "\n";
						
					for (i = 0; i < sum; ++i)
					{
						if (id !== tmp.substring(0, 6))
						{
							slavesAddr += tmp.substring(0, 6);
						}
		
						tmp = tmp.substring(6);
					}			
				}
				
				sendHttpResponse_TEXT(response, tmp, 200);
				
				fs.writeFile('./conf/slaves_addr/aisle_00', slavesAddr, function(){
				
					var restart = spawn('/bin/sh', ['start'], {stdio:'inherit'});

						restart.once('close', function(code){
	
							console.log("restart ok! ");	
					});
				});
				
				
			});

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









