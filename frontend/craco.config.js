const InlineChunkHtmlPlugin = require('react-dev-utils/InlineChunkHtmlPlugin');
const HtmlWebpackPlugin = require('html-webpack-plugin');

// This file configures craco to override defaults in
// create-react-app's webpack.config.js without ejecting.

class HtmlToHppPlugin {
  apply(compiler) {
    compiler.hooks.afterEmit.tap('HtmlToHppPlugin', (compilation) => {
      const fs = require('fs');

      const htmlFilePath = 'build/index.html';
      const hppFilePath = 'build/index_html.hpp';

      // const htmlContent = fs.readFileSync(htmlFilePath, 'utf8');

      // const chunks = [];
      // const maxLength = 70;

      // for (let i = 0; i < htmlContent.length; i += maxLength) {
      //   let chunk = htmlContent.substr(i, maxLength);
      //   chunk = chunk.replace(/"/g, '\\"').replace(/\n/g, '\\n');  // escape quotes and \n
      //   chunks.push(chunk);
      // }

      const htmlContent = fs.readFileSync(htmlFilePath, 'utf8');
      const escapedContent = htmlContent.replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\n/g, '\\n');

      const maxLength = 80;
      const chunks = [];
      let currentChunk = '';

      for (let i = 0; i < escapedContent.length; i++) {
        currentChunk += escapedContent[i];

        if (currentChunk.length >= maxLength && escapedContent[i] !== '\\') {
          chunks.push(currentChunk);
          currentChunk = '';
        }
      }

      if (currentChunk.length > 0) {
        chunks.push(currentChunk);
      }

      const headerFileContent = `#pragma once

#include <string>

const std::string INDEX_HTML_CONTENT =
${chunks.map((chunk) => `"${chunk}"`).join('\n')};
      `;

      fs.writeFileSync(hppFilePath, headerFileContent);

      console.log(`Generated C++ header file: ${hppFilePath}`);
    });
  }
}



module.exports = {
  webpack: {
    plugins: {
      add: [
        [new HtmlToHppPlugin(), 'append']
      ]
    },
    configure: (webpackConfig, { env, paths }) => {

      webpackConfig.plugins.forEach(plugin => {
        // Include all JS files inside the HTML, not just the webpack runtime.
        if (plugin instanceof InlineChunkHtmlPlugin) {
          plugin.tests = [/.+[.]js/]
        }
        // Inject JS in the body instead of head
        if (plugin instanceof HtmlWebpackPlugin) {
          plugin.userOptions.inject = 'body'
        }
      })

      // Pack the CSS inside index.html
      const oneOfRuleIdx = webpackConfig.module.rules.findIndex(rule => !!rule.oneOf);
      webpackConfig.module.rules[oneOfRuleIdx].oneOf.forEach(loader => {
        if (loader.test && loader.test.test && (loader.test.test("test.module.css") || loader.test.test("test.module.scss"))) {
          loader.use.forEach(use => {
            if (use.loader && use.loader.includes('mini-css-extract-plugin')) {
              use.loader = require.resolve('style-loader');
            }
          })
        }
      })

      return webpackConfig
    }
  },
}