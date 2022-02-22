# compile for arm

mkdir build

cmake -DARM_BOARD=true ..

make

make install

# usage
pub -t vc100/cmd/TEST-001 -j '{"msg_id":"1","msg_name":"Set","msg_body":{"Data":{"Name":"vc1000","DevId":"TEST-001","Types":"type1|type2","Areas":"area1|area2","Audio":{"Volumn":100},"Server":{"Ip":"192.168.3.104","Port":1883}}}}'

pub -t vc100/cmd/TEST-001 -j '{"msg_id":"1","msg_name":"PlayAudio","msg_body":{"url":"rtmp://1.1.1.1/aa"}}'

pub -t vc100/cmd/TEST-001 -j '{"msg_id":"1","msg_name":"StopAudio","msg_body":{"url":"rtmp://1.1.1.1/aa"}}'


connect -s 192.168.3.104 -p 1883

