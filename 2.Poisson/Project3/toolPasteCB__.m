function toolPasteCB__(varargin)
hpolys = evalin('base', 'hpolys');
roi = hpolys(1).Position();
targetPosition = roi + ceil(hpolys(2).Position - roi);
im1 = evalin('base', 'im1');
im2 = evalin('base', 'im2');

global cnt;
global p_in;
global id;
global dAS;

[h1, w1, dim1] = size(im1);
g=im2double(im2)*255;
f_=im2double(im1)*255;

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
            %混合梯度：
            for d=1:dim1
                grad_f_=f_(tx,ty,d)-f_(tx+wx(k),ty+wy(k),d);
                grad_g=g(x,y,d)-g(x+wx(k),y+wy(k),d);
                if abs(grad_f_)>abs(grad_g)
                    b(i,1,d)=b(i,1,d)- (grad_g) +(grad_f_);
                end
            end
        end
    end
end
%f=eye(cnt,cnt)/A*b;
f=zeros(cnt,1,dim1);
for o=1:dim1
    f(:,1,o)=dAS\b(:,1,o);
end

for i=1:cnt
    imdst(p_in(i,1)+to(1),p_in(i,2)+to(2),:)=floor(f(i,1,:)+0.5);
end

himg = evalin('base', 'himg');
set(himg, 'CData', imdst);
end

