function im2 = RBFImageWarp(im, psrc, pdst)

% input: im, psrc, pdst

%fprintf("line 1: (%.4f,%.4f) -> (%.4f,%.4f)\n",psrc(1,1),psrc(1,2),pdst(1,1),pdst(1,2));
%fprintf("line 2: (%.4f,%.4f) -> (%.4f,%.4f)\n",psrc(2,1),psrc(2,2),pdst(2,1),pdst(2,2));

n = size(psrc,1);
%交换横纵坐标
P=zeros(n,2);P(:,1)=psrc(:,2);P(:,2)=psrc(:,1);
Q=zeros(n,2);Q(:,1)=pdst(:,2);Q(:,2)=pdst(:,1);

Q=Q-P;

B = zeros(n,n);
for i=1:n
    for j=1:n
        B(i,j)=calc_b(P(i,:),P(j,:));
    end
end

%A=eye(n)/B*Q;
A=Gause(B,Q,n);

%% basic image manipulations
% get image (matrix) size
[h, w, dim] = size(im);

im2 = im;
%% TODO: compute warpped image
pan=zeros(h,w);
%fprintf("dim=%d\n",dim);
for i=1:h
    for j=1:w
        p_now=[i,j];p_to=[i,j];
        for o=1:2
            for k=1:n
                p_to(o)=p_to(o)+A(k,o)*calc_b(p_now(:),P(k,:));
            end
        end
        tx=floor(p_to(1)+0.5);ty=floor(p_to(2)+0.5);
        %fprintf("(%d,%d)->(%d,%d)\n",i,j,tx,ty);
        if tx>0&&tx<=h&&ty>0&&ty<=w
            im2(tx,ty,:)=im(i,j,:);pan(tx,ty)=1;
        end
    end
end
for i=1:h
    for j=1:w
        if pan(i,j)==0
            %取相邻点进行修复
            %if i<h&&pan(i+1,j)==1
            %    im2(i,j,:)=im2(i+1,j,:);pan(i,j)=1;
            %elseif j<w&&pan(i,j+1)==1
            %    im2(i,j,:)=im2(i,j+1,:);pan(i,j)=1;
            %elseif i>1&&pan(i-1,j)==1
            %    im2(i,j,:)=im2(i-1,j,:);pan(i,j)=1;
            %elseif j>1&&pan(i,j-1)==1
            %    im2(i,j,:)=im2(i,j-1,:);pan(i,j)=1;
            %else 
            %end
            cnt=0;
            if i<h&&pan(i+1,j)==1 cnt=cnt+1; end
            if j<w&&pan(i,j+1)==1 cnt=cnt+1; end
            if i>1&&pan(i-1,j)==1 cnt=cnt+1; end
            if j>1&&pan(i,j-1)==1 cnt=cnt+1; end
                
            color=zeros(1,dim);
            for o=1:dim
                if i<h&&pan(i+1,j)==1 color(1,o)=color(1,o)+im2(i+1,j,o)/cnt; end
                if j<w&&pan(i,j+1)==1 color(1,o)=color(1,o)+im2(i,j+1,o)/cnt; end
                if i>1&&pan(i-1,j)==1 color(1,o)=color(1,o)+im2(i-1,j,o)/cnt; end
                if j>1&&pan(i,j-1)==1 color(1,o)=color(1,o)+im2(i,j-1,o)/cnt; end
            end

            im2(i,j,:)=color(:);pan(i,j)=1;
        end
    end
end

