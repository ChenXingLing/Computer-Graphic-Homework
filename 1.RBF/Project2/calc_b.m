function ans_b = calc_b(p1,p2)
dis_S2=calc_dis(p1,p2);
d=500;
ans_b = 1.0/(dis_S2+d);
%ans_b=2.718281828^(-dis_S2/100);
end

