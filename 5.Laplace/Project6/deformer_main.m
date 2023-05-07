%comserver register % make sure Matlab is registered in Windows Registry

enableservice('AutomationServer', true)   % so that the current Matlab session can accept engine connection
% start the C++ program 
!start glvu_matlab.exe  

