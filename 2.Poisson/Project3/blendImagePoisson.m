function blendImagePoisson(im1, im2, roi)

% input: im1 (background), im2 (foreground), roi (in im2), targetPosition (in im1)

[h2, w2, dim2] = size(im2);
%fprintf("h1=%d,w1=%d,dim1=%d\n",h1,w1,dim1);
%fprintf("h2=%d,w2=%d,dim2=%d\n",h2,w2,dim2);

n = size(roi,1);
%% 交换横纵坐标
P=zeros(n+1,2);P(1:n,1)=roi(:,2);P(1:n,2)=roi(:,1);P(n+1,:)=P(1,:);

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

%% 计算
%g :im2 前景(x,y)
%f*:im1 背景(tx,ty)
%f :    函数(x,y)
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
end



