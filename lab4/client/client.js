const prompt=require("prompt-sync")({sigint:true})
const { default: axios } = require("axios")
const CryptoJS = require("crypto-js")
const fs = require('fs')

function getRandNum(min,max) {
	return Math.floor(Math.random()*(max-min))+min;
}

const userName = prompt("请输入用户名:")
const password = prompt("请输入密码:")

const hash1 = CryptoJS.SHA1(userName+password)
console.log('用户名与密码的散列值:',hash1.toString());

const randNum = getRandNum(1000,9999)
console.log("随机生成的四位数是:",randNum);


const hash2 = CryptoJS.SHA1(hash1+randNum.toString())

const sendData={
	userName:userName.toString(),
	hash2:hash2.toString(),
	randNum:randNum.toString()
}



axios.post("http://localhost:8080/auth",sendData)
	.then((res)=>{
		if(res.data.code==1){
			let token = res.data.token
			const decData=CryptoJS.AES.decrypt(res.data.data,hash1.toString(),{
				mode: CryptoJS.mode.CBC,
				padding: CryptoJS.pad.Pkcs7,
				iv: CryptoJS.enc.Utf8.parse("1234567890123456"),
			})
			const decryptedRandNum = CryptoJS.enc.Utf8.stringify(decData);
			// 将数据追加到 verifyRes.txt 文件中
			fs.writeFile('verifyRes.txt', decryptedRandNum, { flag: 'a', encoding: 'utf8' }, (err) => {
				if (err) throw err;
			});
			if(decryptedRandNum==randNum.toString()) {
				fs.writeFile('verifyRes.txt', "\n服务器身份验证通过!\n", { flag: 'a', encoding: 'utf8' }, (err) => {
					if (err) throw err;
				});
				console.log("服务器身份验证通过!")
				let confirm = prompt("是否进行密码修改?(y/n)")

				if(confirm==='y'){
					//修改密码
					var uName=prompt("请输入用户名:")
					var mPwd=prompt("请输入修改后的密码:")

					const hash3 = CryptoJS.SHA1(uName.toString()+mPwd.toString())

					var newData = {
						token:token,
						userName:uName,
						pwd_hash:hash3.toString()
					}

					axios.post("http://localhost:8080/modifyPwd",newData)
						.then((res)=>{
							console.log(res.data)
						})
						.catch(err=>console.log(err.message))
				}
				else {
					return
				}
			}
			else {
				fs.writeFile('verifyRes.txt', "\n服务器身份验证失败!\n", { flag: 'a', encoding: 'utf8' }, (err) => {
					if (err) throw err;
				});
				console.log("服务器身份验证失败!");
			}
		}
		else if(res.data.code==0){
			console.log(res.data.data)
			fs.writeFile('verifyRes.txt', `\n${res.data.data}\n`, { flag: 'a', encoding: 'utf8' }, (err) => {
				if (err) throw err;
			});
		}
	})
	.catch(err=>console.log("出错了"));







