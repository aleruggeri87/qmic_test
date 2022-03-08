%%% qmic_data_usage_example

%% open decoded data
base_path='..\';
fid_ts=fopen([base_path 'decoded_ts_out.dat'],'rb');
fid_addr=fopen([base_path 'decoded_addr_out.dat'],'rb');

ts=fread(fid_ts, inf,'*uint64');
addr=fread(fid_addr, inf,'*uint16');

fclose(fid_ts);
fclose(fid_addr);

if length(ts) ~= length(addr)
    error('mismatched data in the files!');
end

%% plot a subset of the data
pr_len=1000; % how many events to plot
if length(ts) < pr_len
    pr_len = length(ts);
end

figure(1)
subplot(2,1,1)
plot(addr(1:pr_len))
ylabel('Pixel number (#)')

subplot(2,1,2)
plot(ts(1:pr_len))
ylabel('Timestamp (2 ns)')
xlabel('Event number (#)')

%% hit 2d map
figure(2)
draw_colormap(addr,true); % second parameter: enable/disable pixel number on map
colorbar

%% print coincidences
% ts array is sorted, so it's easy to find coincidences: simply look for
% consecutive elements which have the same value.
analize_N_events=100e3;       % number of events to be analyzed
%analize_N_events=length(ts);

home
first=true;
old_ts=ts(1);
% this prints to screen even coincidences which involves > 2 events
for k=2:analize_N_events
    if ts(k)==old_ts
        if first
            fprintf('at %8.3f us clicked pixels %d, %d', ...
                double(old_ts)*2e-3, addr(k-1), addr(k));
            first=false;
        else
            fprintf(', %d', addr(k));
        end
    else
        old_ts=ts(k);
        if ~first
            fprintf('.\n');
            first=true;
        end
    end
end

%% Create coincidence matrix - "C style" code
NPIX=576;
CM=zeros(NPIX); % empty coincidence matrix (NPIX x NPIX)
tic
for k=2:length(ts) % for each event
    if ts(k-1)==ts(k) % find if consecutive events are at the same time
        % increment coincidence matrix element by 1
        CM(addr(k-1)+1,addr(k)+1) = CM(addr(k-1)+1,addr(k)+1) + 1;
        % WARNING: in this simple code multiple coincidences are considered
        % only in pairs:
        % if A, B and C are the involved addresses, only A-B and B-C events
        % are added to the CM matrix, A-C is not considered!
    end
end
toc % extremely slow approach in matlab... very fast in C!

% since addresses are not guaranteed to be sorted in coincidences, CM
% matrix is not exactly upper triangular. Code below produce a full matrix
for k=1:NPIX
    for j=k:NPIX
        CM(k,j)=CM(k,j)+CM(j,k);
        CM(j,k)=CM(k,j);
    end
end

% show resulting coincidence matrix
figure(3)
imagesc(CM,'AlphaData',CM~=0), axis image


%% Create coincidence matrix - faster MATLAB approach
CM=zeros(NPIX); % empty coincidence matrix (NPIX x NPIX)
tic
idx=find(diff(ts)==0); % as before only consecutive coincidences are considered!
for k=1:length(idx)
    CM(addr(idx(k))+1,addr(idx(k)+1)+1) = CM(addr(idx(k))+1,addr(idx(k)+1)+1) + 1;
end
toc
CM=CM+CM.'; % make full matrix

% show resulting coincidence matrix
figure(4)
imagesc(CM,'AlphaData',CM~=0), axis image

%% Use coincidence matrix to show coincidences given by 1 pixel
A=176; % aggressor pixel

XT=reshape(CM(A+1,:),24,24)'; % select the proper row (+1 for matlab addressing)
figure(5)
imagesc(XT, 'AlphaData', XT~=0)
axis image
set(gca,'Xdir','reverse')
set(gca,'color',0*[1 1 1]);
colorbar
