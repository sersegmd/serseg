'use strict';
const vscode = require('vscode');
const cp = require('child_process');

function activate(context) {
  const output = vscode.window.createOutputChannel('SERSEG');
  const diagnostics = vscode.languages.createDiagnosticCollection('serseg');
  const sequences = new Map();
  const bySource = new Map();
  function publish(key, uri, items) {
    if (uri) bySource.set(key, { uri, items }); else bySource.delete(key);
    const groups = new Map();
    for (const entry of bySource.values()) {
      const id = entry.uri.toString();
      if (!groups.has(id)) groups.set(id, { uri: entry.uri, items: [] });
      groups.get(id).items.push(...entry.items);
    }
    diagnostics.clear();
    for (const entry of groups.values()) diagnostics.set(entry.uri, entry.items);
  }
  let warned = false;
  context.subscriptions.push(output, diagnostics);
  function check(document) {
    if (document.languageId !== 'serseg' || document.uri.scheme !== 'file' || document.isDirty) return;
    const key = document.uri.toString();
    const seq = (sequences.get(key) || 0) + 1;
    sequences.set(key, seq);
    cp.execFile('serseg', ['check', document.fileName], { timeout: 30000, maxBuffer: 1024 * 1024 }, (error, stdout, stderr) => {
      if (sequences.get(key) !== seq) return;
      if (error && error.code === 'ENOENT') {
        publish(key);
        if (!warned) { warned = true; vscode.window.showWarningMessage('SERSEG compiler was not found on PATH. Install it and restart VS Code.'); }
        return;
      }
      const text = String(stderr || '');
      const header = text.match(/Error\[([^\]]+)\]: ([^\r\n]+)/);
      const location = text.match(/--> (.+):(\d+):(\d+)/);
      if (header && location) {
        const uri = vscode.Uri.file(location[1]);
        const line = Math.max(0, Number(location[2]) - 1);
        const column = Math.max(0, Number(location[3]) - 1);
        const item = new vscode.Diagnostic(new vscode.Range(line, column, line, column + 1), header[2], vscode.DiagnosticSeverity.Error);
        item.code = header[1]; item.source = 'serseg';
        publish(key, uri, [item]);
      } else {
        publish(key);
        if (error) { output.appendLine(text || error.message); output.show(true); }
      }
    });
  }
  function task(command) {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== 'serseg' || editor.document.uri.scheme !== 'file') {
      vscode.window.showWarningMessage('Open a SERSEG file first.'); return;
    }
    const scope = vscode.workspace.getWorkspaceFolder(editor.document.uri) || vscode.TaskScope.Workspace;
    const execution = new vscode.ProcessExecution('serseg', [command, editor.document.fileName], { cwd: scope.uri ? scope.uri.fsPath : undefined });
    const t = new vscode.Task({ type: 'serseg', command }, scope, `SERSEG: ${command}`, 'SERSEG', execution, []);
    t.presentationOptions = { reveal: vscode.TaskRevealKind.Always, panel: vscode.TaskPanelKind.Dedicated, clear: true };
    vscode.tasks.executeTask(t);
  }
  context.subscriptions.push(vscode.commands.registerCommand('serseg.build', () => task('build')));
  context.subscriptions.push(vscode.commands.registerCommand('serseg.run', () => task('run')));
  context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(check));
  context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(check));
  context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(doc => { sequences.delete(doc.uri.toString()); publish(doc.uri.toString()); }));
  for (const doc of vscode.workspace.textDocuments) check(doc);
}
exports.activate = activate;
function deactivate() {}
exports.deactivate = deactivate;
