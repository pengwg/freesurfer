function [voxshift, err] = tdr_peshift(volref,krvol,tol)
% voxshift = tdr_peshift(volref,krvol,<tol>)
%
% volref is the reference volume (already fully reconned)
% krvol is the movable volume with its rows reconned
%   but not its columns (see tdr_recon_rows).
%
% tol is the tolerance. Default is .05
%
%
% $Id: tdr_peshift.m,v 1.1 2003/10/28 04:30:07 greve Exp $


voxshift = [];
if(nargin ~= 2)
  fprintf('voxshift = tdr_peshift(volref,krvol)\n');
  return;
end
if(exist('tol') ~= 1) tol = .05; end
nmax = 30;

tic;

% Get a bracket for the minimum (hopefully) %
ashift = -20;
cshift = +10;
bshift =  ashift + .61803399*(cshift-ashift);

krvolshift = tdr_kshift(krvol,ashift);
vol = abs(tdr_recon_cols(krvolshift));
aerr = sum(reshape1d(volref-vol).^2);

krvolshift = tdr_kshift(krvol,bshift);
vol = abs(tdr_recon_cols(krvolshift));
berr = sum(reshape1d(volref-vol).^2);

krvolshift = tdr_kshift(krvol,cshift);
vol = abs(tdr_recon_cols(krvolshift));
cerr = sum(reshape1d(volref-vol).^2);

xshift = gr1dmin_update(ashift,bshift,cshift);

tolcheck = cshift-ashift;

nth = 1;
while(tolcheck > 2*tol & nth <= nmax)
  
  krvolshift = tdr_kshift(krvol,xshift);
  vol = abs(tdr_recon_cols(krvolshift));
  xerr = sum(reshape1d(volref-vol).^2);
  
  [xshift ashift bshift cshift aerr berr cerr] = ...
   gr1dmin_update(ashift, bshift, cshift, aerr, berr, cerr,xshift,xerr);
    
  err = [aerr berr cerr xerr];
  shift = [ashift bshift cshift xshift];
  [minerr iminerr] = min(err);
  minerrshift = shift(iminerr);
  fprintf('n = %2d, tol = %6.4f, shift = %7.4f, err = %7.4f (%g)\n',...
	  nth,cshift-ashift,minerrshift,minerr*1e7,toc);
  tolcheck = cshift-ashift;
  nth = nth + 1;
end

if(nth > nmax)
  fprintf('WARNING: search timed out\n');
end

voxshift = minerrshift;

return;


for voxshift = [-2:.1:0]
%for voxshift = 0
  krvolshift = tdr_kshift(krvol,voxshift);
  vol = abs(tdr_recon_cols(krvolshift));
  err = sum(reshape1d(volref-vol).^2);
  fprintf('voxshift = %5.2f, err = %g, toc = %g\n',voxshift,err*1e7,toc);
end



return;

