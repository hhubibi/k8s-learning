## 容器操作

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

## k8s
首先使用minikube创建一个单节点集群
```
minukube start
```

为了让shell使用minikube的docker-daemon，需要设置一下环境变量，不然就去docker pull拉镜像，一直失败
```
eval $(minikube -p minikube docker-env)
```

然后在这个环境用构建镜像
```
docker build -t helloworld .
```

`helloworld.yaml`的文件如下，这里必须为pod打上标签，不然会没有办法暴露服务。
```
apiVersion: v1
kind: Pod
metadata:
  name: helloworld-demo
  labels:
    app: helloworld   # 添加的标签
spec:
  containers:
  - name: helloworld-demo
    image: helloworld:latest
    imagePullPolicy: Never
    ports:
    - containerPort: 8080
```

然后启动pod
```
kubectl apply -f helloworld.yaml
```

暴露服务
```
kubectl expose po helloworld-demo --type=LoadBalancer --name helloworld-http

$ kubectl get services
NAME              TYPE           CLUSTER-IP      EXTERNAL-IP   PORT(S)          AGE
hello-minikube    NodePort       10.98.168.203   <none>        8080:30629/TCP   36d
helloworld-http   LoadBalancer   10.107.53.104   <pending>     8080:32695/TCP   61s
kubernetes        ClusterIP      10.96.0.1       <none>        443/TCP          36d
```

helloworld-http 服务是一个 LoadBalancer 类型的服务，它有一个pending的外部 IP 地址，但是我们还不能通过这个地址直接访问服务。

一种方法是通过 Minikube 提供的 IP 地址和服务的 NodePort 来访问该服务。
```
$ kubectl get services
NAME              TYPE           CLUSTER-IP       EXTERNAL-IP   PORT(S)          AGE
hello-minikube    NodePort       10.98.168.203    <none>        8080:30629/TCP   36d
helloworld-http   LoadBalancer   10.101.252.206   <pending>     8080:31567/TCP   12s
kubernetes        ClusterIP      10.96.0.1        <none>        443/TCP          36d

$ minikube ip
192.168.49.2

$ curl 192.168.49.2:31567
<h3>Hello Test!</h3><b>Hostname:</b> helloworld-demo<br/>%
```