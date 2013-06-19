- To build on Windows:

  ```
  vcbuild.bat
  ```
  node.exe will be in /Release

- To run the image encryption demo:
  - add the node bin folder(/Release) to your path
  - run
      ```
      cd example_parallel/image encryption demo/

      node --expose_rivertrail test_encryption.js
      
      node --expose_rivertrail test_decryption.js
      ```
- Tips:
  - This repo is based on node-v0.10.12 from [here](http://nodejs.org/download/)
  - This demo relies on [node-pngjs](https://github.com/niegowski/node-pngjs/) module. It can be installed by `npm install pngjs`. I just copied a `node_modules` folder here.
  - If node says `comprehension failed: SyntaxError: Illegal token`, try change the eol marker of js files to UNIX, namely change `CR+LF` to `LF`.
  - If you want to try the sequential mode, change `this.data = imageEncryption_p(this);` to `this.data = imageEncryption(this);` in `test_encryption.js`. Same for decryption.
