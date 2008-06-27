% Steven Herbst
% herbst@mit.edu

% Newton-Raphson solution of an NPN inverter

clear all;
close all;

Vt=1/40;    % thermal voltage
Va=10;	    % Early voltage
Is=1e-14;  % saturation current

% forward beta
bF=100;

% reverse beta
bR=1;

Vcc=5.0;    % supply voltage
gPU=1/10000.0;   % conductance of pull-up resistor

e = 1e-4;   % error tolerance
dampStep = 0.025; % maximum change in vBE per iteration
max_iters=1000;

N=100;
vIN=linspace(0,1.0,N)';
vOUT=zeros(N,1);
iters=zeros(N,1);

% node aliases for transistor
b = 2;
c = 3;

for i=1:N
	x = [Vcc;vIN(i);Vcc/2;0.0;0.0]; % initial guesses for node voltages and supply currents

	dx = e*[1;1;1;1;1];

	count = 0;
	while (count<max_iters) && (norm(dx,2)>e)
	    count = count+1;

	    % generate small-signal conductance matrix
	    %    1      2       3       4       5
	    A = [gPU	0	-gPU	1	0; ... % 1
		 0	0	0	0	1; ... % 2
		 -gPU	0	gPU	0	0; ... % 3
		 1	0	0	0	0; ... % 4
		 0	1	0	0	0];    % 5
	    
	    % generate input vector
	    z = [0;       ... % 1
		 0;       ... % 2
		 0;       ... % 3
		 Vcc;     ... % 4
		 vIN(i)];     % 5

	    % create model for transistor

	    Ibf = (Is/bF)*(exp(x(b)/Vt)-1);
	    Ibr = (Is/bR)*(exp((x(b)-x(c))/Vt)-1);

	    Icc = bF*Ibf-bR*Ibr;

	    gmF = bF*Ibf/Vt;
	    gmR = bR*Ibr/Vt;

	    gPiF = gmF/bF;
	    gPiR = gmR/bR;

	    Ibf_eq = Ibf - gPiF*x(b);
	    Ibr_eq = Ibr - gPiR*(x(b)-x(c));
	    Icc_eq = Icc - gmF*x(b) + gmR*(x(b)-x(c));
				
	    % stamp in transistor
	    
	    % forward parameters
	    A(c,b) = A(c,b) + gmF;
	    A(b,b) = A(b,b) + gPiF;

	    % reverse parameters
	    A(c,b) = A(c,b) + -gmR;
	    A(c,c) = A(c,c) + gmR;

	    A(b,b) = A(b,b) + gPiR;
	    A(b,c) = A(b,c) + -gPiR;
	    A(c,b) = A(c,b) + -gPiR;
	    A(c,c) = A(c,c) + gPiR;
	    
	    % bias currents

	    z(b) = z(b) + -Ibf_eq;

	    z(b) = z(b) + -Ibr_eq;
	    z(c) = z(c) + Ibr_eq;

	    z(c) = z(c) + -Icc_eq;
	    
	    % solve A*x=z
	    
	    dx = A\z-x;
	    
	    for j=1:5
		if (abs(dx(j)) > dampStep)
			dx(j) = sign(dx(j))*dampStep;
		    end
	    end
	    
	    x = x+dx;
	end

	vOUT(i)=x(c);
	iters(i)=count;
end

figure;

plot(vIN,vOUT,'r-;;');
title('Transfer Characteristic of an NPN Inverter');
xlabel('vIN');
ylabel('vOUT');

figure;

plot(vIN,iters,'r-;;');
xlabel('# N-R Iterations');
ylabel('vIN');
