# parking
一个简单的停车场计费系统

### 功能：
* GEC6818开发板连接两台RFID读卡器，一台作为车辆进场读卡，一台作为车辆出场读卡
* 刷卡成功，长哔一声（0.5秒）；刷卡失败，短哔五声（0.1秒）
* 车辆进场刷卡成功时，启动照相机拍照，并将照片路径、当前时间和卡号录入数据库
* 车辆出场刷卡成功时，自动计算停车费
* 已经入场的RFID不可重复入场，已经出场的RFIF卡不可重复出场
 
### 硬件平台：
* GEC-6818
* ARM-Cortex A53
* Mifare522 module超高频RFID读卡器
* RFID（S50）白卡
 
### 软件平台：
* linux-3.4.39
* libc-2.23

### 技术讨论群：<a href="//shang.qq.com/wpa/qunwpa?idkey=bc2c3338276a40ac72131230ad041a00c60a2fe45172ab6b9a93fea44cf0e6fa">![image](https://github.com/vincent040/lab/blob/master/res/QQ_qun.png?raw=true)
