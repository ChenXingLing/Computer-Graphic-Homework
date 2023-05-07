function toolPasteCB(varargin)
hpolys = evalin('base', 'hpolys');
roi = hpolys(1).Position();

im1 = evalin('base', 'im1');
im2 = evalin('base', 'im2');

blendImagePoisson(im1, im2, roi);

toolPasteCB__();

addlistener(hpolys(1), 'MovingROI' , @toolPasteCB);%拖动左边则重新计算dAS

addlistener(hpolys(2), 'MovingROI' , @toolPasteCB__);%拖动右边则直接计算b和f


