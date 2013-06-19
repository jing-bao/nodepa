var PNG = require('pngjs').PNG;
var fs = require('fs');
eval(fs.readFileSync('./jslib/jit/narcissus/jsdefs.js').toString());
eval(fs.readFileSync('./jslib/jit/narcissus/jslex.js').toString());
eval(fs.readFileSync('./jslib/jit/narcissus/jsparse.js').toString());
eval(fs.readFileSync('./jslib/jit/narcissus/jsdecomp.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/definitions.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/helper.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/dotviz.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/rangeanalysis.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/inferblockflow.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/infermem.js').toString());
eval(fs.readFileSync('./jslib/ParallelArray.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/typeinference.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/driver.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/genOCL.js').toString());
eval(fs.readFileSync('./jslib/jit/compiler/runOCL.js').toString());
eval(fs.readFileSync('./imageDecryption.js').toString());

fs.createReadStream('encryption.png')
    .pipe(new PNG({
        filterType: 4
    }))
    .on('parsed', function() {	
		
		t1 = new Date().getTime();
		this.data = imageDecryption_p(this);// or imageDecryption
		t2 = new Date().getTime();
		console.log("Execution time is " + (t2-t1)/1000 + "s");
		
        this.pack().pipe(fs.createWriteStream('decryption.png'));
    });
