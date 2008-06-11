% Steven Herbst
% herbst@mit.edu

% Uses Newton-Raphson iteration to find the 
% transfer caracteristic for a simple inverter

% N-R parameters
% MOSFETs are quadratic, so
% damped iteration is unnecessary

e=1e-3;

% MOSFET parameters

Vth = 0.5;	% threshold voltage
Va  = 5;	% Early voltage
K   = 1e-3;	% K-value, in A/(V^2)

% Voltage supply
Vdd = 5;

% Pull-up resistor
gr = 1/10000;

Vin=0:0.1:Vdd;
Vout=zeros(1,length(Vin));

% indices for gate-to-source voltages and
% drain-to-source voltages
% see the declaration of the G matrix below

gs=2;
ds=3;

% Loop over input voltages
count = 0;
for	Vgs=Vin
	count = count + 1;

	% N-R iteration for each input voltage

	% initial the solution vector
	% there are 5 unknowns: three node voltages
	% and two branch currents
	v  = zeros(5,1);
	dv = 2*e*ones(5,1);
	while norm(dv,2) > e

		% Initialize G
		% Note the additional two rows and columns
		% to accomodate voltage sources

		G =	[gr	0	-gr	1	0;
			 0	0	0	0	1;
			 -gr	0	gr	0	0;
			 1	0	0	0	0;
			 0	1	0	0	0;];

		% Initialize current vector

		i =	[0;
			 0;
			 0;
			 Vdd;
			 Vgs];

		% set Id,gm,and go based on operating region

		if ((v(gs)-Vth) < 0)
		% cutoff
			Id=0;
			gm=0;
			go=0;
		elseif ((v(gs)-Vth) < v(ds))
		% saturation
			Id=K/2*(v(gs)-Vth)^2;
			gm=sqrt(2*K*Id);
			go=Id/Va;
		else 
		% triode
			Id=K*((v(gs)-Vth)-v(ds)/2)*v(ds);
			gm=K*v(ds);
			go=K*((v(gs)-Vth)-v(ds));
		end

		% stamp is gm and go

		G(3,2) = G(3,2) + gm;
		G(3,3) = G(3,3) + go;

		% stamp in equivalen current source

		i(3) = i(3)-(Id-v(gs)*gm-v(ds)*go);

		% solve the matrix equation
		dv = (G^-1)*i-v;
		v  = v + dv;

	end

	Vout(count) = v(ds);
end

plot(Vin,Vout,Vth*ones(length(Vin),1),Vin,Vin,Vin-Vth);
title('MOSFET Inverter Transfer Characteristic');
xlabel('Vin');
ylabel('Vout');
ylim([0 Vdd]);
