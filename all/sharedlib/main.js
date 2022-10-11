const fs = require('fs');
const path = require('path');
const inquirer = require('inquirer');
const generator = require('./generator');

const DEFAULT_SHAREDLIBS_DIR = path.join(__dirname, '..');

function validateReg(text, reg, prompt) {
  const regExp = new RegExp(reg);
  if (regExp.test(text)) {
    return true;
  }
  return `${prompt}(${reg})`;
}

function makePrompt() {
  const INPUT_SHAREDLIB_NAME_TIP = 'Please input valid sharedlib name';
  const INPUT_SHAREDLIB_DIR_TIP = 'Please input valid sharedlib directory';
  const prompts = [{
    type: 'input',
    message: 'Please input sharedlib name: ',
    name: 'name',
    validate: val => validateReg(val, '^[a-z][_a-z]*[a-z]+$', INPUT_SHAREDLIB_NAME_TIP),
  }];
  prompts.push({
    type: 'input',
    message: `Please input sharedlib dir: (${DEFAULT_SHAREDLIBS_DIR})`,
    name: 'sharedlibDir',
    validate: inputDir => {
      if (inputDir.trim() === '' || fs.statSync(inputDir).isDirectory()) {
        return true;
      }
      return INPUT_SHAREDLIB_DIR_TIP;
    },
  });
  return prompts;
}

async function main() {
  try {
    const answers = await inquirer.prompt(makePrompt());
    if (answers.sharedlibDir.trim() === '') {
      answers.sharedlibDir = DEFAULT_SHAREDLIBS_DIR;
    }

    generator.run(answers);
  } catch (error) {
    console.warn(error);
  }
}

main();
