var imageEncryption = function(__src){
	var sData = __src.data,
		width = __src.width,
		height = __src.height,
		pi = Math.PI;
		dData = new Array(width*height*4),
		pos = [1.2, 7.1, 3.4, 1.7, 4.6, 6.3, 2.7, 5.5, 1.6,
			   8.9, 4.7, 1.3, 1.1, 3.2, 6.4, 1.3, 9.5, 5.2],
		wl = [5, 6, 7, 8, 9, 10, 11, 12, 13],
		A = [300, 320, 340, 360, 380, 400, 420, 440, 460],	
		pos_xy = new Array(18);
	
	for (var i = 0; i < 9; i++) {
		pos_xy[2*i] = Math.round(height/pos[2*i]);
		pos_xy[2*i+1] = Math.round(width/pos[2*i+1]);
	}
		
	var d = new Array(9);
	var	wave = new Array(9);

	for(y = height - 1 ; y >= 0; y--){
		for(x = width - 1; x >= 0; x--){
			var offset = (y * width + x) * 4;
			for(var i = 0; i < 9; i++) {
				d[i] = Math.sqrt((x-pos_xy[2*i+1])*(x-pos_xy[2*i+1])+(y-pos_xy[2*i])*(y-pos_xy[2*i]));
				wave[i] = Math.round(A[i]*Math.sin(2*pi*d[i]/wl[i]));
			}
			dData[offset] = (sData[offset] + wave[0] + wave[1] + wave[2] + 2560)%256; 
			dData[offset + 1] = (sData[offset + 1] + wave[3] + wave[4] + wave[5] + 2560)%256;
			dData[offset + 2] = (sData[offset + 2] + wave[6] + wave[7] + wave[8] + 2560)%256;		
			dData[offset + 3] = 255;
		}
	}
	return dData;
};

function __kernel_E(index, _sData, _width, _pos_xy, _wl, _A, _pi)
{	
	var _x = index[1];
	var _y = index[0];
	var d = [0, 0, 0, 0, 0, 0, 0, 0, 0];
	var wave = [0, 0, 0, 0, 0, 0, 0, 0, 0];
	var tmp = [255, 255, 255, 255];

	for(var i = 0; i < 9; i++) {
		d[i] = Math.sqrt((_x-_pos_xy[2*i+1])*(_x-_pos_xy[2*i+1])+(_y-_pos_xy[2*i])*(_y-_pos_xy[2*i]));
		wave[i] = Math.round(_A[i]*Math.sin(2*_pi*d[i]/_wl[i]));
	}
	
	var offset = (_y * _width + _x) * 4;
	tmp[0] = (_sData[offset] + wave[0] + wave[1] + wave[2] + 2560)%256; 
	tmp[1] = (_sData[offset + 1] + wave[3] + wave[4] + wave[5] + 2560)%256;
	tmp[2] = (_sData[offset + 2] + wave[6] + wave[7] + wave[8] + 2560)%256;
	tmp[3] = 255;	
	return tmp;	
}

var imageEncryption_p = function(__src){
	var sData = __src.data,
		width = __src.width,
		height = __src.height,
		pi = Math.PI;
	var size = width*height*4;
	var tmpArray = new Array(size);
	for (var i = 0; i < size; i++) tmpArray[i] = sData[i];
	var	pos = [1.2, 7.1, 3.4, 1.7, 4.6, 6.3, 2.7, 5.5, 1.6,
			   8.9, 4.7, 1.3, 1.1, 3.2, 6.4, 1.3, 9.5, 5.2],
		wl = [5, 6, 7, 8, 9, 10, 11, 12, 13],
		A = [300, 320, 340, 360, 380, 400, 420, 440, 460],	
		pos_xy = new Array(18);

	for (var i = 0; i < 9; i++) {
		pos_xy[2*i] = Math.round(height/pos[2*i]);
		pos_xy[2*i+1] = Math.round(width/pos[2*i+1]);
	}
  
	var dData = new ParallelArray([height, width], __kernel_E, tmpArray, width, pos_xy, wl, A, pi).data;
	return dData;
};