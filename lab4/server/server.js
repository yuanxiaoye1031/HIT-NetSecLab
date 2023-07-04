const express = require('express')
const app = express()
const server = require('http').createServer(app)
const bodyParser = require('body-parser')
app.use(bodyParser.json())
const CryptoJS = require("crypto-js")
const mysql = require('mysql2');

function getByUserName(userName,callback){
	const connection = mysql.createConnection({
		host: 'localhost', // 连接本机
		user: 'root',      // 数据库用户名
		password: '123456', // 数据库密码
		database: 'company',   // 数据库名称
	});
	  
	connection.connect((err) => {
		if (err) {
			console.error('Failed to connect to MySQL database.', err);
		} else {
			// 执行查询语句
			// const sql = 'SELECT * FROM user where `username` = '+userName.toString()
			const sql = `SELECT * FROM user WHERE username = '${userName.toString()}'`
			connection.query(sql, (err, results) => {
				if (err) throw err;  // 查询失败，抛出异常
	
				// console.log(results);  // 打印查询结果
				callback(results)
			});
		}
	});
}

app.post("/auth",(req,res)=>{

	// console.log(req.body)

	// 提取收到的数据（收到的均为字符串类型）
	const userName = req.body.userName
	const hash2 = req.body.hash2
	const randNum = req.body.randNum
	
	getByUserName(userName,(results)=>{
		const hash1 = results[0].pwd_hash
		console.log("hash1值为:",hash1);

		const myHash2 = CryptoJS.SHA1(hash1+randNum)

		console.log("hash2:\t",hash2);
		console.log("myHash2\t",myHash2.toString())
		if(hash2==myHash2.toString()){
			console.log("用户身份验证通过!");
			const encData = CryptoJS.AES.encrypt(randNum,hash1,{
				mode: CryptoJS.mode.CBC,
				padding: CryptoJS.pad.Pkcs7,
				iv: CryptoJS.enc.Utf8.parse("1234567890123456"),
			}).toString()

			const sendData = {
				code:1,
				data:encData,
				token:'qwertyuiop'
			}

			res.send(sendData)
		}
		else {
			console.log("用户身份验证失败!")
			const sendData={
				code:0,
				data:"用户名或密码错误"
			}
			res.send(sendData)
		}
	})
})

app.post("/modifyPwd",(req,res)=>{

	// console.log(req.body)

	let token = req.body.token
	let uName = req.body.userName
	let pwd_hash=req.body.pwd_hash
	console.log(pwd_hash)

	if(token=='qwertyuiop'){
		const connection = mysql.createConnection({
			host: 'localhost', // 连接本机
			user: 'root',      // 数据库用户名
			password: '123456', // 数据库密码
			database: 'company',   // 数据库名称
		});
		  
		connection.connect((err) => {
			if (err) {
				console.error('Failed to connect to MySQL database.', err);
			} else {
				// 执行查询语句
				// const sql = 'SELECT * FROM user where `username` = '+userName.toString()
				const sql = `UPDATE user SET pwd_hash = '${pwd_hash}' WHERE username = '${uName}'`
				connection.query(sql, (err, results) => {
					if (err) throw err;  // 查询失败，抛出异常
		
					// console.log(results);  // 打印查询结果
					res.send("修改成功!")
					return 
				});
			}
		});
	}

	else{
		res.send("修改失败!")
	}

})

const port =process.env.PORT || 8080

server.listen(port,() => {
	console.log('服务器运行成功，正在监听',port,"端口")
})

