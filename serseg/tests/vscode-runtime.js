'use strict';
const assert = require('assert');
const Module = require('module');
const realLoad = Module._load;
const callbacks = [], commands = {}, events = {}, entries = new Map();
const document = path => ({ languageId:'serseg', uri:{ scheme:'file', toString:()=>path, fsPath:path }, fileName:path, isDirty:false });
const vscode = {
  Uri: { file: path => ({ fsPath:path, toString:()=>path }) },
  Diagnostic: class { constructor(range, message, severity) { Object.assign(this,{ range, message, severity }); } },
  Range: class { constructor(...args) { this.args=args; } },
  DiagnosticSeverity: { Error: 0 },
  languages: { createDiagnosticCollection: () => ({ clear:()=>entries.clear(), set:(uri,items)=>entries.set(uri.toString(),items), dispose(){} }) },
  window: { createOutputChannel:()=>({appendLine(){},show(){},dispose(){}}), showWarningMessage:()=>{}, activeTextEditor:null },
  workspace: { textDocuments:[], onDidSaveTextDocument:fn=>(events.save=fn,{dispose(){}}), onDidOpenTextDocument:fn=>(events.open=fn,{dispose(){}}), onDidCloseTextDocument:fn=>(events.close=fn,{dispose(){}}), getWorkspaceFolder:()=>undefined },
  commands: { registerCommand:(name,fn)=>(commands[name]=fn,{dispose(){}}) },
  tasks:{executeTask:()=>{}}, TaskScope:{Workspace:1}, TaskRevealKind:{Always:1}, TaskPanelKind:{Dedicated:1}
};
const child = {execFile:(cmd,args,options,callback)=>{assert.equal(cmd,'serseg');callbacks.push(callback);}};
Module._load = function(request, parent, main) { if (request==='vscode') return vscode; if(request==='child_process') return child; return realLoad.apply(this,arguments); };
require('../editors/vscode/extension.js').activate({ subscriptions:[] });
const a=document('/a.ser'), b=document('/b.ser');
events.open(a);
callbacks.shift()({},'', 'Error[E102]: undefined\n  --> /module.ser:2:3\n');
assert.equal(entries.get('/module.ser')[0].code,'E102');
events.save(a); events.save(a);
const older=callbacks.shift(), newer=callbacks.shift();
newer(null,'',''); older({},'', 'Error[E101]: stale\n  --> /module.ser:1:1\n');
assert.equal(entries.size,0,'stale diagnostic must not reappear');
events.open(a); events.open(b);
callbacks.shift()({},'', 'Error[E102]: a\n  --> /module.ser:2:3\n');
callbacks.shift()({},'', 'Error[E102]: b\n  --> /module.ser:2:3\n');
assert.equal(entries.get('/module.ser').length,2);
events.close(a); assert.equal(entries.get('/module.ser').length,1);
events.close(b); assert.equal(entries.size,0);
assert(commands['serseg.build'] && commands['serseg.run']);
console.log('PASS extension diagnostics lifecycle, stale results and commands (mocked VS Code)');
