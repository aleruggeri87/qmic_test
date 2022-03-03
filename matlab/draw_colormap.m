function H=draw_colormap(pixels, display_nums)
    %% colormap(pixels, display_nums)
    H=hist(pixels,0:575);
    H=reshape(H,24,24)';
    imagesc(H, 'AlphaData', H~=0)  % pixels with 0 counts will be black
    axis image
    set(gca,'Xdir','reverse')
    set(gca,'color',0*[1 1 1]);
    if nargin > 1 && display_nums
        PXN=(reshape(0:575,24,24)');
        imagesc_overlay_numbers(gca,PXN);
    end
end

function imagesc_overlay_numbers(fig_hndl, numbers)
    img=findobj(fig_hndl, 'type', 'image');
    L=length(img.CData);
    CM=colormap;
    CM_range=caxis;
    
    for k=1:numel(img.CData)
        x=img.CData(k);

        str=sprintf('%.0f',numbers(k));
        if x>mean(CM_range)
            col='k';
        else 
            col='w';
        end
        th=text(floor((k-1)/L)+1,rem(k-1,L)+1,...
            str,'HorizontalAlignment','Center','Color',col);
        th.FontSize=6;

    end
end