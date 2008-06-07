% Steven Herbst
% herbst@mit.edu

% Newton-Raphson solution of a circuit
% consisting of current source in parallel
% with a resistor and a branch containing two
% identical diodes in series
 
Vt=1/40;    % thermal voltage
Is=1e-14;   % reverse bias current

I=20e-3;    % supply current
gr=1/100;   % conductance of resistor

v = [0; 0];      % initial voltage guess
e = 1e-6;   % error tolerance

dv = [inf ; inf];   % change in voltage calculated by N-R
dv_max = 0.2;       % maximum change in voltage per iteration

count = 0;

while norm(dv,2)>e
    count = count + 1;

    % generate small-signal conductance matrix
    
    G = [gr, 0; 0, 0];  % initialize with a resistor in 
                        % parallel with the current source
                       
    % create small signal models for diodes
                        
    id1 = Is*(exp((v(1)-v(2))/Vt)-1);
    gd1 = Is*exp((v(1)-v(2))/Vt)/Vt;
    
    id2 = Is*(exp(v(2)/Vt)-1);
    gd2 = Is*exp(v(2)/Vt)/Vt;
    
    % stamp in diode from node 1 to node 2 (diode 1)
    
    G = G + [gd1, -gd1; -gd1, gd1];
    
    % stamp in diode from node 2 to ground (diode 2)
    
    G = G + [0, 0; 0, gd2];
    
    % initialize current supply vector
    
    i = [I ; 0];
    
    % stamp in bias currents
    
    i = i + [-v(1)*gr ; 0]; % resistor
    i = i + [-id1; id1];    % diode 1
    i = i + [0; -id2];      % diode 2
    
    % solve G*dv=i
    
    dv = (G^-1)*i;
    
    if (abs(dv(1)) > dv_max)
        dv(1) = sign(dv(1))*dv_max;
    end
    
    if (abs(dv(2)) > dv_max)
        dv(2) = sign(dv(2))*dv_max;
    end
    
    v = v+dv;
end

% Display information about the simulation

Ir=v(1)*gr;
Id1=Is*(exp((v(1)-v(2))/Vt)-1);
Id2=Is*(exp(v(2)/Vt)-1);

v

fprintf(1,'Iterations: %d\n',count);
fprintf(1,'Ir: %d\n',Ir);
fprintf(1,'Id1: %d\n',Id1);
fprintf(1,'Id2: %d\n',Id2);
fprintf(1,'Error current in node 1: %d\n',I-Ir-Id1);
fprintf(1,'Error current in node 2: %d\n',Id1-Id2);
