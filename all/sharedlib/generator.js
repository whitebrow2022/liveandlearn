const path = require('path');
const fs = require('fs');
const humps = require('humps');
const languageEncoding = require("detect-file-encoding-and-language");
const {spawnSync, exec} = require('child_process');
const moment = require('moment');
const config = require('./config');

const whiteList = ['.gitkeep'];
const blackList = ['build', 'Build', 'vs_example.sln', '.idea', '.gradle', '.DS_Store', 'captures', '.externalNativeBuild', '.cxx', 'local.properties', 'MacApp.xcworkspace', 'MacExample.xcodeproj', 'IosApp.xcworkspace'];

/**
 * Collect a list of files to be processed. 
 * @param {path} dir 
 * @param {Array} fileList 
 * @param {boolean} recursive 
 */
function collectFileList(dir, fileList, recursive = true) {
  fs.readdirSync(dir).forEach(fileName => {
    if (blackList.includes(fileName)) {
      return;
    }
    const fileNameWithoutExt = path.parse(fileName).name;
    if (fileNameWithoutExt.length === 0 && !whiteList.includes(fileName)) {
      return;
    }
    const filePath = path.resolve(dir, fileName);
    const stat = fs.statSync(filePath);
    if (stat.isFile()) {
      fileList.push(filePath);
    }
    if (recursive && stat.isDirectory()) {
      collectFileList(filePath, fileList, recursive);
    }
  });
}

function makeNewPath(templatePath, name, humpName, shrinkName) {
  return templatePath
      .replace(config.UNDERSCORE_LOWERCASE_NAME_REG, name)
      .replace(config.UNDERSCORE_HUMP_NAME_REG, humpName)
      .replace(config.UNDERSCORE_SHRINK_NAME_REG, shrinkName)
      .replace(config.LOWERCASE_NAME_REG, name)
      .replace(config.SHRINK_NAME_REG, shrinkName)
      .replace(config.HUMP_NAME_REG, humpName);
}

async function getFileEncoding(f) {
  const encodingResult = await languageEncoding(f);
  if (encodingResult.encoding === 'UTF-8') {
    return 'utf8';
  }
  return '';
}

async function generate(srcFullPath, destFullPath, params) {
  try {
    const relativePath = path.relative(params.sharedlibDir, destFullPath);
    console.log(`Generate file: ${relativePath}`);

    const destDir = path.dirname(destFullPath);
    if (!fs.existsSync(destDir)) {
      fs.mkdirSync(destDir, {recursive: true});
    }

    const extName = path.extname(relativePath);
    
    if (extName === ".ico" || extName === ".png") {
      fs.copyFileSync(srcFullPath, destFullPath);
      return;
    }

    // console.log(`${srcFullPath} check encoding`);
    const encoding = await getFileEncoding(srcFullPath);
    // console.log(`${srcFullPath} use '${encoding}'`);
    if (encoding == 'utf8') {
      srcContent = fs.readFileSync(srcFullPath, 'utf-8').toString();
    } else {
      console.error(`${srcFullPath} use '${encoding}'`);
      return;
    }

    const fileContent = srcContent
        .replace(config.PERCENT_USERNAME_REG, params.userName)
        .replace(config.PERCENT_DATE_REG, params.date)
        .replace(config.PERCENT_YEAR_REG, params.year)
        .replace(config.PERCENT_HUMP_NAME_REG, params.humpName)
        .replace(config.UNDERSCORE_LOWERCASE_NAME_REG, params.name)
        .replace(config.UNDERSCORE_HUMP_NAME_REG, params.humpName)
        .replace(config.DASH_HUMP_NAME_REG, params.humpName)
        .replace(config.LOWERCASE_NAME_REG, params.name)
        .replace(config.UPPERCASE_NAME_REG, params.upperName)
        .replace(config.SHRINK_NAME_REG, params.shrinkName)
        .replace(config.HUMP_NAME_REG, params.humpName)
        .replace(config.LITTLE_HUMP_NAME_REG, params.littleHumpName)
        .replace(config.LITTLE_DASH_HUMP_NAME_REG, params.littleDashHumpName);

    if (encoding == 'utf8') {
      fs.writeFileSync(destFullPath, fileContent);
    } else if (encoding == 'utf16le') {
      const buffer = Buffer.from(fileContent, 'utf16le')
      fs.writeFileSync(destFullPath, buffer);
    }
    if (extName == '.sh') {
      exec(`chmod +x ${destFullPath}`, (error, stdout, stderr) => {
        if (error) {
          console.log(`error: ${error.message}`);
          return;
        }
        if (stderr) {
          console.log(`stderr: ${stderr}`);
          return;
        }
        //console.log(`stdout: ${stdout}`);
      });
    }
  } catch (error) {
    console.warn(`Generate file failed: ${error}`);
  }
}

function run(params) {
  try {
    params.upperName = params.name.toUpperCase();
    params.shrinkName = params.name.replace("_", "");
    params.humpName = humps.pascalize(params.name);
    params.littleHumpName = humps.camelize(params.name);
    params.littleDashHumpName = params.name.replace("_", "-");

    try {
      const gitPipeline = spawnSync('git', ['config', '--get', 'user.name']);
      params.userName = gitPipeline.stdout.toString().trim();
      if (typeof params.userName !== 'string' || params.userName.length === 0) {
        params.userName = 'Anonymous';
      } 
    } catch (error) {
      params.userName = 'Anonymous';
    }

    params.date = moment(new Date()).format('yyyy/MM/DD');
    params.year = params.date.substr(0, 4);

    const listOfFilePath = [];
    collectFileList(config.TEMPLATE_DIR, listOfFilePath);
    listOfFilePath.forEach(srcFilePath => {
      const relativePath = path.relative(config.TEMPLATE_DIR, srcFilePath);
      const destRelativePath = makeNewPath(relativePath, params.name, params.humpName, params.shrinkName);
      const destFilePath = path.resolve(path.join(params.sharedlibDir, params.name, destRelativePath));
      generate(srcFilePath, destFilePath, params);
    });
  } catch (error) {
    console.warn('Generate sharedlib failed:', error)
  }
}

module.exports = {run};
