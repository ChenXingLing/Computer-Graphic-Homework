figure; 
t = 0:0.001:1;
%%
h = drawpolyline;
hold on;
hcurve = plot(bezier(h.Position, t), 'g', 'linewidth', 2);
h.addlistener('MovingROI', @(h, evt) bezier(evt.CurrentPosition, t, hcurve));

%%
function p = bezier(A, t, h)
    n=size(A,1);p=zeros(size(t,2),2);P=A;%P=zeros(n,2);
    for o=1:size(t,2)
        u=t(1,o);
        %tmp=A;
        for i=1:n-1
            for j=1:n-i
                P(j,:)=(1-u)*P(j,:)+u*P(j+1,:);
            end
            %tmp=P;
        end
        p(o,:)=P(1,:);%取出u处点
    end
    p = p*[1;1i];
    if nargin>2,set(h, 'xdata', real(p), 'ydata', imag(p)); end
end

