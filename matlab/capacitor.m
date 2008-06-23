% Steven Herbst
% herbst@mit.edu

C=1e-9;
R=1000;

dt=R*C/100;
N=1000;

t=0:dt:(N-1)*dt;
v=zeros(1,N);

v(1) = 0; % set initial capacitor voltage

V = 5;	% supply voltage;

for i=2:N
	% generate companion model
	gc = C/dt;
	I = gc*v(i-1);

	% solve single variable equation
	v(i)=(V/R+I)/(1/R+gc);
end

plot(t,v);
title("RC circuit step response");
xlabel("time");
ylabel("voltage");
