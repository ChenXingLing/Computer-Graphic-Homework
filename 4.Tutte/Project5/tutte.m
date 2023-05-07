[x,f]=readObj('cathead');%x为点，f为每个三角面的三点序号
m=size(x,1);%m:点总个数
e=zeros(m,m);%邻接矩阵
for i=1:size(f,1)
    e(f(i,1),f(i,2))=1;e(f(i,2),f(i,1))=1;
    e(f(i,1),f(i,3))=1;e(f(i,3),f(i,1))=1;
    e(f(i,2),f(i,3))=1;e(f(i,3),f(i,2))=1;
end

A=zeros(m,m);b=zeros(m,2);
for i=1:m
    cnt=0;
    for j=1:m
        if e(i,j)==1
            cnt=cnt+1;
            A(i,j)=1;
        end
    end
    A(i,:)=A(i,:)/cnt;
    A(i,i)=-1;
end
B=findBoundary(x,f);n=size(B,2);%边界点个数
theta=2.0*pi/n;
for i=1:n%遍历边界点
    b(B(i),:)=[cos(i*theta) sin(i*theta)];%单位圆边界
    A(B(i),:)=0;A(B(i),B(i))=1;
end
%drawmesh(f,x);
drawmesh(f,A\b);