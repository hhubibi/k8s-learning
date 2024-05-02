构建镜像
```
docker build -t helloworld .
```

运行容器
```
docker run -p 9080:8080 helloworld
```

测试
```
curl http://localhost:9080
```

给容器镜像起名
```
docker tag helloworld hhu/helloworld:v1

$ docker images
REPOSITORY       TAG       IMAGE ID       CREATED         SIZE
hhu/helloworld   v1        c647d4d41109   5 minutes ago   1.03GB
helloworld       latest    c647d4d41109   5 minutes ago   1.03GB
```

挂载例子
```
$ docker run -d -v /test helloworld               
3e84ef849a804b57ea9bf8fbe5cdc15c005841d46dbec958f83fa351e59b743b

$ docker volume ls
DRIVER    VOLUME NAME
local     4c92fd466de1298b639f0b9402283703b1144eb518a4a1a38b37092cf29aeee2

$ docker exec -it 3e84ef849a80 /bin/bash     
root@3e84ef849a80:/app# cd /test
root@3e84ef849a80:/test# ls
root@3e84ef849a80:/test# echo "hello world" > test.txt
root@3e84ef849a80:/test# ls
test.txt

# 这里的_data文件夹是容器的volumn在宿主机上对应的临时目录，这个文件并不会被提交
$ ls /var/lib/docker/volumes/4c92fd466de1298b639f0b9402283703b1144eb518a4a1a38b37092cf29aeee2/_data/
test.txt

# 在宿主机上查看容器的可读写层
$ cd /var/lib/docker/image/overlay2/layerdb/mounts/3e84ef849a804b57ea9bf8fbe5cdc15c005841d46dbec958f83fa351e59b743b

$ ls
init-id  mount-id  parent

$ cat mount-id 
49e5bae7f3b372bcb1ab4548101f61f943c3f42f2a2a1b8d91af6c38a7234a93

$ cd /var/lib/docker/overlay2/49e5bae7f3b372bcb1ab4548101f61f943c3f42f2a2a1b8d91af6c38a7234a93/merged
$ ls
app  bin  boot  dev  etc  home  lib  lib64  media  mnt  opt  proc  root  run  sbin  srv  sys  test  tmp  usr  var
```