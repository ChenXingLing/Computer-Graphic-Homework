## **【报告】Poisson Image Editing**

### **1.【概括】**

Matlab 实现 Poisson 图像融合算法。

使用大型稀疏线性系统求解方法（基于稀疏矩阵分解），实时拖动多边形区域得到结果。

### **2.【算法介绍】**

#### **(1).【概述】**

用 $f*(p)$ 表示背景，$g(p),p\in S$ 表示前景，$\Omega\sub S$ 为前景中所截取部分的点集，$N_p$ 表示离散点 $p$ 上下左右四个方向的点，$\partial \Omega=\left\{p \in S \backslash \Omega: N_p \cap \Omega \neq \emptyset\right\}$

现求未知函数 $f$ 表示成图。

有关系式：$\text { for all } p \in \Omega, \quad\left|N_p\right| f_p-\sum_{q \in N_p \cap \Omega} f_q=\sum_{q \in N_p \cap \partial \Omega} f_q^*+\sum_{q \in N_p} v_{p q}$，其中 $v_{pq}=g_p-g_q$

令 $\boldsymbol A=(a_{ij})_{|\Omega|\times |\Omega|}$，$\boldsymbol f=(f_{i})_{|\Omega|\times 1}$，$\boldsymbol b=(b_{i})_{|\Omega|\times 1}$，解方程 $\boldsymbol {Af}=\boldsymbol {b}$ 即可。



#### **(2).【获取离散点坐标】**

取出 $\Omega$ 区域 $y$ 值的范围，然后依次枚举 $y$，求得该水平线上与 $\Omega$ 边界的若干交点，即 $\partial \Omega$ 离散点集。对于凹多边形，可能有多个交点。按 $x$ 排序后，每相邻的两个点中间的即为内点。

```python
%% 获取离散边界点坐标
miy=floor(min(P(:,2)));
may=ceil(max(P(:,2)));
if miy<1
    miy=1;
end
if may>w2
    may=w2;
end
pox=zeros(may,n);m=zeros(1,may);
global cnt;cnt = 0;
for y=miy:may
    for i=1:n
        tmpx=P(i,1)+(P(i+1,1)-P(i,1))*(y-P(i,2))/(P(i+1,2)-P(i,2));
        %fprintf("tmpx=%.1f\n",tmpx);
        if (tmpx>=P(i,1)&&tmpx<=P(i+1,1))||(tmpx>=P(i+1,1)&&tmpx<=P(i,1))
            m(1,y)=m(1,y)+1;
            pox(y,m(1,y))=floor(tmpx+0.5);
        end
    end
    if m(1,y)>0
        %y
        %pox(y,:)
        pox(y,1:m(1,y))=sort(pox(y,1:m(1,y)));
        %pox(y,:)
        i=1;
        while i<m(1,y)
            if pox(y,i)<pox(y,i+1)
                cnt=cnt+pox(y,i+1)-pox(y,i)+1-2;
            end
            i=i+2;
        end
        
    end
end
```

```python
%% 获取离散内部点坐标
global p_in;p_in=zeros(cnt,2);
global id;id=zeros(h2,w2);
tt=0;
for y=miy:may
    i=1;
    while i<m(1,y)
        if pox(y,i)+1<pox(y,i+1)%内部：如果有点
            for x=pox(y,i)+1:pox(y,i+1)-1
                tt=tt+1;
                p_in(tt,1)=x;
                p_in(tt,2)=y;
                id(x,y)=tt;
            end
        end
        i=i+2;
    end
end
```

#### **(3).【计算矩阵A】**

对 $\boldsymbol A$ 使用稀疏矩阵和 $\text{Cholesky}$ 预分解加速运算。

```python
%% 计算
wx=[-1,1,0,0];wy=[0,0,-1,1];
A=zeros(cnt,cnt);
for i=1:cnt
    A(i,i)=4;%|N(p)|f(p)
    x=p_in(i,1);y=p_in(i,2);
    for k=1:4
        if id(x+wx(k),y+wy(k))>0%如果q是内点
            A(i,id(x+wx(k),y+wy(k)))=-1;%-f(q)
        end
    end
end
AS=sparse(A);
global dAS;
dAS=decomposition(AS,'chol');
```

#### **(4).【计算b】**

计算 $\Omega$ 在背景中摆放的位置，求得 $\boldsymbol f$ 后逐一对应，

```python
imdst = im1;%拷贝背景f*
to=[floor(targetPosition(1,2)-roi(1,2)+0.5),floor(targetPosition(1,1)-roi(1,1)+0.5)];
b=zeros(cnt,1,dim1);
wx=[-1,1,0,0];wy=[0,0,-1,1];
for i=1:cnt
    x=p_in(i,1);y=p_in(i,2);
    tx=x+to(1);ty=y+to(2);
    %fprintf("4%.1f-%.1f-%.1f-%.1f-%.1f\n",g(x,y,o),g(x-1,y,o),g(x+1,y,o),g(x,y-1,o),g(x,y+1,o));
    vg=4.0*g(x,y,:)-g(x-1,y,:)-g(x+1,y,:)-g(x,y-1,:)-g(x,y+1,:);%v(p,q)=\sum (g(p)-g(q))
    b(i,1,:)=vg;

    for k=1:4
        if tx+wx(k)>=0&&tx+wx(k)<=h1&&ty+wy(k)>=1&&ty+wy(k)<=w1
            if id(x+wx(k),y+wy(k))==0%如果q是外点
                b(i,1,:)=b(i,1,:)+f_(tx+wx(k),ty+wy(k),:);%f*(q)
            end
        end
    end
end
```

### **3.【混合梯度优化】**

完成上述算法得到如下结果

![](./src/_1.png)

下面重新定义 $v_{p q}= \begin{cases}f_p^*-f_q^* & \text { if }\left|f_p^*-f_q^*\right|>\left|g_p-g_q\right| \\ g_p-g_q & \text { otherwise, }\end{cases}$

```python
			%混合梯度：
            for d=1:dim1
                grad_f_=f_(tx,ty,d)-f_(tx+wx(k),ty+wy(k),d);
                grad_g=g(x,y,d)-g(x+wx(k),y+wy(k),d);
                if abs(grad_f_)>abs(grad_g)
                    b(i,1,d)=b(i,1,d)- (grad_g) +(grad_f_);
                end
            end
```

得到更自然的融合：

![](./src/_2.png)

### 4.【实时拖动绘制】

将“计算b”和绘制图像的过程放入函数 `toolPasteCB__`，在 `tooPasteCB_` 中调用一次，再对多边形位置进行监听、回调。

```python
function toolPasteCB(varargin)
hpolys = evalin('base', 'hpolys');
roi = hpolys(1).Position();

im1 = evalin('base', 'im1');
im2 = evalin('base', 'im2');

blendImagePoisson(im1, im2, roi);

toolPasteCB__();

addlistener(hpolys(1), 'MovingROI' , @toolPasteCB);%拖动左边则重新计算dAS

addlistener(hpolys(2), 'MovingROI' , @toolPasteCB__);%拖动右边则直接计算b和f

```

传参使用全局变量。