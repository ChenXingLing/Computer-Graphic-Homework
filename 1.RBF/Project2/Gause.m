function ans_A = Gause(B,Q,n)
A=zeros(n,n+2);
A(1:n,1:n)=B(1:n,1:n);
A(1:n,n+1:n+2)=Q(1:n,1:2);
for j=1:n
    w=j;
    for i=j+1:n
        if A(w,j)<A(i,j)
            w=j;
        end
    end
    tmp=A(j,:);A(j,:)=A(w,:);A(w,:)=tmp;
    for i=1:n
        if i~=j
            temp=A(i,j)/A(j,j);
            A(i,j:n+2)=A(i,j:n+2)-A(j,j:n+2)*temp;
        end
    end
end
ans_A=A(1:n,n+1:n+2);
for i=1:n
    ans_A(i,1)=ans_A(i,1)/A(i,i);
    ans_A(i,2)=ans_A(i,2)/A(i,i);
end
end

