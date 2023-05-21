const InlineChunkHtmlPlugin = require('react-dev-utils/InlineChunkHtmlPlugin');
const HtmlWebpackPlugin = require('html-webpack-plugin');

// This file configures craco to override defaults in
// create-react-app's webpack.config.js without ejecting.

module.exports = {
  webpack: {
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