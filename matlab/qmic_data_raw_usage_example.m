%%% qmic_data_raw_usage_example
%% open decoded raw data
base_path='..\';
fid_ts=fopen([base_path 'decoded_ts_out.dat'],'rb');
fid_addr=fopen([base_path 'decoded_addr_out.dat'],'rb');

ts=fread(fid_ts, inf,'*int64');
addr=fread(fid_addr, inf,'*uint16');

fclose(fid_ts);
fclose(fid_addr);

if length(ts) ~= length(addr)
    error('mismatched data in the files!');
end

%% hit 2d map
figure(1)
draw_colormap(addr,true); % second parameter: enable/disable pixel number on map
colorbar

%% find coincidences between 1 selected pixel and all the others
aggressor=176;

tdc_val=int16(bitand(ts,255));    % lower 8-bits. Convert these to signed ints
upper_val=bitshift(ts,-8); % since ts is unsorted and diff can produce
i=find(diff(upper_val)==0);       % negative numbers!
% i contain indexes of potential correlation events, since they have the
% upper part of the timestamp equal

c=0; % counter of potential correlation which includes the aggressor
px=[]; % this array will contain number of victim pixels.
dt=[]; % distance between arrival time of aggressor and of the victim
for k=1:length(i)
    if addr(i(k)) == aggressor || addr(i(k)+1) == aggressor
        % ...if aggressor is one of the two pixels
        c=c+1;
        dt(c)=tdc_val(i(k))-tdc_val(i(k)+1); % calculate time difference
        if dt(c)==0 % time difference == 0 -> coincidence
            %fprintf('%d <-> %d\n',addr(i(k)),addr(i(k)+1)); % uncomment to
                                                  % see the list of pixel pairs
            if addr(i(k)) == aggressor % save the address of the victim pixel
                px(c)=addr(i(k)+1)+1;  % this is saved incremented by one
            else                       % to simplify the elimination of 
                px(c)=addr(i(k))+1;    % events which do not involve aggressor
            end
        end
    end
end
px=px(px~=0)-1; % eliminate events which do not involve aggressor and subtract 1
fprintf('%d events involve px %d and another one within +/-400ns\n',c, aggressor)

figure(2)
draw_colormap(px,true); % map of coincidence events
figure(3)
hist(double(dt),-220:220) % distribution of events. Peak at zero are coincidences
xlim([-220 220])
