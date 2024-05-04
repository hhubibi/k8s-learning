

创建虚拟磁盘
```
qemu-img create -f qcow2 ubuntu-vm1.qcow2 30G
```

创建虚拟机
```
qemu-system-x86_64 \
  -m 4G -smp 4 \
  -cpu host,vmx=on \
  -hda ubuntu-vm1.qcow2 \
  -cdrom ubuntu-22.04.4-live-server-amd64.iso \
  -boot d \
  -enable-kvm \
  -nic bridge,br=br0,model=virtio-net-pci \
  -daemonize
```

启动虚拟机
```
qemu-system-x86_64 \
  -m 4G -smp 4 \
  -cpu host,vmx=on \
  -hda ubuntu-vm1.qcow2 \
  -enable-kvm \
  -net nic,macaddr=52:54:00:12:34:57 -net bridge,br=br0 \
  -daemonize
```

安装必要的包
[install-kubeadm](https://v1-29.docs.kubernetes.io/docs/setup/production-environment/tools/kubeadm/install-kubeadm/)

```
sudo apt-get update
# apt-transport-https may be a dummy package; if so, you can skip that package
sudo apt-get install -y apt-transport-https ca-certificates curl gpg

# If the directory `/etc/apt/keyrings` does not exist, it should be created before the curl command, read the note below.
# sudo mkdir -p -m 755 /etc/apt/keyrings
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.29/deb/Release.key | sudo gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg

# This overwrites any existing configuration in /etc/apt/sources.list.d/kubernetes.list
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.29/deb/ /' | sudo tee /etc/apt/sources.list.d/kubernetes.list

sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl docker.io
sudo apt-mark hold kubelet kubeadm kubectl
```

复制虚拟磁盘
```
cp -av ubuntu-vm1.qcow2 ubuntu-vm2.qcow2
```

虚拟网络
```
sudo apt-get install bridge-utils

sudo ip link add name br0 type bridge

# sudo
ip addr add 192.168.100.1/24 brd + dev br0
ip link set br0 up
# 开启 ip_forward
sysctl -w net.ipv4.ip_forward=1
# 允许对从 br0 流入的数据包进行 FORWARD
iptables -t filter -A FORWARD -i br0 -j ACCEPT
iptables -t filter -A FORWARD -o br0 -j ACCEPT
# 也可以直接将 filter FORWARD 策略直接设置为 ACCEPT
# iptables -t filter -P FORWARD ACCEPT
# 开启 NAT
iptables -t nat -A POSTROUTING -o eno1 -j MASQUERADE

# 进入虚拟机后配置网卡
ip addr add 192.168.100.2/24 brd + dev ens3
ip route add default via 192.168.100.1 dev ens3

# 按需修改 /etc/resolv.conf 配置 DNS 服务器，不改的话只能ping ip地址
nameserver 8.8.8.8
```

如果想要持久化
需要修改`/etc/netplan/00-installer-config.yaml`文件，不然每次重启都会失效
```
network:
  ethernets:
    ens3:
      #dhcp4: true
      addresses: [192.168.100.2/24]
      nameservers:
        addresses: [8.8.8.8]
      routes:
       - to: default
         via: 192.168.100.1
  version: 2
```

启动另外两个虚拟机
```
sudo qemu-system-x86_64 \
  -m 4G -smp 4 \
  -cpu host,vmx=on \
  -hda ubuntu-vm1.qcow2 \
  -enable-kvm \
  -net nic,macaddr=52:54:00:12:34:57 -net bridge,br=br0 \
  -daemonize

sudo qemu-system-x86_64 \
  -m 4G -smp 4 \
  -cpu host,vmx=on \
  -hda ubuntu-vm2.qcow2 \
  -enable-kvm \
  -net nic,macaddr=52:54:00:12:34:58 -net bridge,br=br0 \
  -daemonize

# addresses: [192.168.100.3/24]

sudo qemu-system-x86_64 \
  -m 4G -smp 4 \
  -cpu host,vmx=on \
  -hda ubuntu-vm3.qcow2 \
  -enable-kvm \
  -net nic,macaddr=52:54:00:12:34:59 -net bridge,br=br0 \
  -daemonize

# addresses: [192.168.100.4/24]
```

k8s换源
```
kubeadm config print init-defaults > kubeadm.conf

# imageRepository: registry.aliyuncs.com/google_containers

kubeadm config images list

kubeadm config images pull --config kubeadm.conf

# 手动打tag
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/coredns:v1.11.1 registry.k8s.io/coredns:v1.11.1
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/etcd:3.5.12-0 registry.k8s.io/etcd:3.5.12-0
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/kube-apiserver:v1.29.0 registry.k8s.io/kube-apiserver:v1.29.0
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/kube-controller-manager:v1.29.0 registry.k8s.io/kube-controller-manager:v1.29.0
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/kube-proxy:v1.29.0 registry.k8s.io/kube-proxy:v1.29.0
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/kube-scheduler:v1.29.0 registry.k8s.io/kube-scheduler:v1.29.0
ctr -n k8s.io i tag registry.aliyuncs.com/google_containers/pause:3.9 registry.k8s.io/pause:3.9

hostnamectl set-hostname k3s-master

# master node
# sudo swapoff -a 需要禁止swap
kubeadm init --config kubeadm.conf
```

其中conf文件内容
```
apiVersion: kubeadm.k8s.io/v1beta3
bootstrapTokens:
- groups:
  - system:bootstrappers:kubeadm:default-node-token
  token: abcdef.0123456789abcdef
  ttl: 24h0m0s
  usages:
  - signing
  - authentication
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 192.168.100.2
  bindPort: 6443
nodeRegistration:
  criSocket: unix:///var/run/containerd/containerd.sock
  imagePullPolicy: IfNotPresent
  name: k3s-master
  taints: null
---
apiServer:
  timeoutForControlPlane: 4m0s
apiVersion: kubeadm.k8s.io/v1beta3
certificatesDir: /etc/kubernetes/pki
clusterName: kubernetes
controllerManager: {}
dns: {}
etcd:
  local:
    dataDir: /var/lib/etcd
imageRepository: registry.aliyuncs.com/google_containers
kind: ClusterConfiguration
kubernetesVersion: 1.29.0
networking:
  dnsDomain: cluster.local
  serviceSubnet: 10.96.0.0/12
  podSubnet: 10.244.0.0/16  # flannel的cidr
scheduler: {}
```

初始化成功后输出信息
```
To start using your cluster, you need to run the following as a regular user:

  mkdir -p $HOME/.kube
  sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

Alternatively, if you are the root user, you can run:

  export KUBECONFIG=/etc/kubernetes/admin.conf

You should now deploy a pod network to the cluster.
Run "kubectl apply -f [podnetwork].yaml" with one of the options listed at:
  https://kubernetes.io/docs/concepts/cluster-administration/addons/

Then you can join any number of worker nodes by running the following on each as root:

kubeadm join 192.168.100.2:6443 --token abcdef.0123456789abcdef \
	--discovery-token-ca-cert-hash sha256:ee49df999fb57ffc9239a754854efc0be50ab9e586b40327c0008508ce8c6dd2 

```

查看镜像
```
# 等价，kubeadm拉的镜像在k8s.io的namespace
ctr -n k8s.io image ls
crictl images
```

结果
```
root@hxy:/home/hxy# kubectl cluster-info
Kubernetes control plane is running at https://192.168.100.2:6443
CoreDNS is running at https://192.168.100.2:6443/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

To further debug and diagnose cluster problems, use 'kubectl cluster-info dump'.
root@hxy:/home/hxy# kubectl get nodes
NAME         STATUS     ROLES           AGE   VERSION
k3s-master   NotReady   control-plane   94s   v1.29.4

root@hxy:/home/hxy# kubectl describe node k3s-master
...
  Ready            False   Fri, 03 May 2024 12:25:05 +0000   Fri, 03 May 2024 12:25:01 +0000   KubeletNotReady              container runtime network not ready: NetworkReady=false reason:NetworkPluginNotReady message:Network plugin returns error: cni plugin not initialized
...
```

问题解决
```
ERRO[0000] validate service connection: validate CRI v1 image API for endpoint "unix:///var/run/dockershim.sock": rpc error: code = Unavailable desc = connection error: desc = "transport: Error while dialing: dial unix /var/run/dockershim.sock: connect: no such file or directory" 
容器运行时错误

# 3个机器都需要做
crictl config runtime-endpoint unix:///var/run/containerd/containerd.sock
crictl config image-endpoint unix:///run/containerd/containerd.sock

detected that the sandbox image "registry.k8s.io/pause:3.8" of the container runtime is inconsistent with that used by kubeadm. It is recommended that using "registry.k8s.io/pause:3.9" as the CRI sandbox image.

sudo mkdir /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml
sudo vim /etc/containerd/config.toml
修改sandbox_image = "registry.aliyuncs.com/google_containers/pause:3.9"
修改cgroup相关的为SystemdCgroup = true
systemctl restart containerd

/etc/sysconfig/kubelet文件内容
KUBELET_EXTRA_ARGS="--fail-swap-on=false --cgroup-driver=systemd"
```

dashboard使用
```
https://github.com/kubernetes/dashboard/blob/master/docs/user/access-control/creating-sample-user.md

kubectl -n kubernetes-dashboard create token admin-user
---
eyJhbGciOiJSUzI1NiIsImtpZCI6IklfY0dQZmNDdjl4WHhmaDVFelhQTUsxb2lXMXcweUs5bkFBWWxGVlJPanMifQ.eyJhdWQiOlsiaHR0cHM6Ly9rdWJlcm5ldGVzLmRlZmF1bHQuc3ZjLmNsdXN0ZXIubG9jYWwiXSwiZXhwIjoxNzE0Nzk4MTgwLCJpYXQiOjE3MTQ3OTQ1ODAsImlzcyI6Imh0dHBzOi8va3ViZXJuZXRlcy5kZWZhdWx0LnN2Yy5jbHVzdGVyLmxvY2FsIiwia3ViZXJuZXRlcy5pbyI6eyJuYW1lc3BhY2UiOiJrdWJlcm5ldGVzLWRhc2hib2FyZCIsInNlcnZpY2VhY2NvdW50Ijp7Im5hbWUiOiJhZG1pbi11c2VyIiwidWlkIjoiYzA5YzA3MTEtNGY2NS00ODkwLWJiNzEtOTFmN2M1OTAwYmQ0In19LCJuYmYiOjE3MTQ3OTQ1ODAsInN1YiI6InN5c3RlbTpzZXJ2aWNlYWNjb3VudDprdWJlcm5ldGVzLWRhc2hib2FyZDphZG1pbi11c2VyIn0.U9Xk3-f1-hcrdnwvJ0IwtvbOSHlSSuLsoS2MQuwV716bcLTXA-6R5WdqLpqGI9lw6vrYtaaQFH8mceWf-8GaR7eCA4R5XCtyJJFa3zR8OuQvSpC9YfrGiwotl8ViZXOROw65q14Q2TAruIzSejckiiGlTrcPODlHOjnvgE02tHluYN6CqK7yTr6RPY9nIjEHMGjtMIF3JqHCS0sYHfUmlVyUVSZBiTULmNpE5Z6nTj10g6TNi-DvKQy4HHvTMv8zJsUo2C9VYHY0eLoXCPhSNcc9pGMdbo6OGL-INUFzV4RiEm6Ttt2bF-DM1pD0Xd-x5YuzbOma9z92FwR9K0rv_w
```

修改之后就可以通过访问https://192.168.100.2:30002来访问dashboard了