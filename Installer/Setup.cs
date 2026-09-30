// Copyright 2026 Hungry Ghost Studios. AGPL-3.0-or-later.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

internal static class Package {
    internal static readonly string Version = Text("version").Trim();
    internal static readonly Dictionary<string,string> Hashes = Text("hashes").Split(new[]{'\n'},StringSplitOptions.RemoveEmptyEntries)
        .Select(line=>line.TrimEnd('\r').Split('\t')).ToDictionary(parts=>parts[0],parts=>parts[1],StringComparer.Ordinal);
    internal static readonly string[] Bundles = Hashes.Keys.Select(path=>path.Split('/')[1]).Distinct().OrderBy(name=>name).ToArray();
    internal static string Text(string resource) { using(var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream(resource)) using(var reader=new StreamReader(stream)) return reader.ReadToEnd(); }
    internal static ZipArchive Open() { return new ZipArchive(Assembly.GetExecutingAssembly().GetManifestResourceStream("payload"),ZipArchiveMode.Read); }
    internal static string Digest(Stream stream) { using(var sha=SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","").ToLowerInvariant(); }
    internal static void Verify() {
        using(var zip=Open()) {
            var files=zip.Entries.Where(entry=>entry.FullName.StartsWith("VST3/",StringComparison.Ordinal)&&entry.Name.Length>0).ToArray();
            if(files.Length!=Hashes.Count || Bundles.Length!=50) throw new InvalidDataException("Incomplete plugin package.");
            foreach(var entry in files) {
                string expected;
                if(!Hashes.TryGetValue(entry.FullName,out expected)) throw new InvalidDataException("Unexpected package file.");
                using(var stream=entry.Open()) if(Digest(stream)!=expected) throw new InvalidDataException("Package checksum mismatch.");
            }
        }
    }
    internal static string Within(string root,string relative) {
        string fullRoot=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
        string full=Path.GetFullPath(Path.Combine(fullRoot,relative.Replace('/',Path.DirectorySeparatorChar)));
        if(!full.StartsWith(fullRoot,StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Invalid package path.");
        return full;
    }
    internal static string Install(string destination,string[] chosen,Action<int,string> progress) {
        Verify();
        string root=Path.GetFullPath(destination);
        if(chosen.Length==0 || chosen.Any(name=>!Bundles.Contains(name))) throw new InvalidDataException("Select a valid plugin.");
        string backup=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"HungryGhostAudio","InstallBackups",DateTime.UtcNow.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N").Substring(0,8));
        var changed=new List<Tuple<string,string>>();
        using(var zip=Open()) try {
            for(int index=0;index<chosen.Length;index++) {
                string prefix="VST3/"+chosen[index]+"/";
                foreach(var entry in zip.Entries.Where(e=>e.FullName.StartsWith(prefix,StringComparison.Ordinal)&&e.Name.Length>0)) {
                    string relative=entry.FullName.Substring(5);
                    string target=Within(root,relative), previous=null;
                    Directory.CreateDirectory(Path.GetDirectoryName(target));
                    if(File.Exists(target)) {
                        previous=Within(backup,relative); Directory.CreateDirectory(Path.GetDirectoryName(previous)); File.Copy(target,previous,false);
                    }
                    using(var input=entry.Open()) using(var output=new FileStream(target,FileMode.Create,FileAccess.Write,FileShare.None)) {
                        changed.Add(Tuple.Create(target,previous));
                        input.CopyTo(output);
                    }
                    using(var output=File.OpenRead(target)) if(Digest(output)!=Hashes[entry.FullName]) throw new IOException("Installed file verification failed.");
                }
                progress((index+1)*100/chosen.Length,chosen[index]);
            }
            string record=Path.Combine(backup,"installation.txt");Directory.CreateDirectory(backup);
            File.WriteAllText(record,"Hungry Ghost Audio "+Version+"\r\nDestination: "+root+"\r\n"+String.Join("\r\n",chosen)+"\r\n",Encoding.UTF8);
            return backup;
        } catch {
            foreach(var item in Enumerable.Reverse(changed)) {
                // All targets originated from Within(root, ...); restore only
                // the exact files touched by this install, never whole folders.
                if(item.Item2!=null) File.Copy(item.Item2,item.Item1,true);
                else if(File.Exists(item.Item1)) File.Delete(item.Item1);
            }
            throw;
        }
    }
}

internal sealed class SetupWindow:Form {
    readonly CheckedListBox selection=new CheckedListBox();
    readonly TextBox destination=new TextBox();
    readonly Label status=new Label(); readonly ProgressBar progress=new ProgressBar();
    readonly Button install=new Button(); readonly Button browse=new Button();
    readonly Button all=new Button(),none=new Button(); bool busy;
    Color paper=Color.FromArgb(221,222,215), jade=Color.FromArgb(148,215,194), panel=Color.FromArgb(20,26,26);
    internal SetupWindow() {
        Text="Hungry Ghost Audio · Windows VST3 setup"; ClientSize=new Size(800,680);
        StartPosition=FormStartPosition.CenterScreen; FormBorderStyle=FormBorderStyle.FixedDialog;
        MaximizeBox=false; BackColor=Color.FromArgb(12,17,17); ForeColor=paper; Font=new Font("Segoe UI",10);
        using(var iconStream=Assembly.GetExecutingAssembly().GetManifestResourceStream("brand-icon")) Icon=new Icon(iconStream);
        using(var markStream=Assembly.GetExecutingAssembly().GetManifestResourceStream("brand-mark")) {
            var mark=new PictureBox{Image=new Bitmap(markStream),SizeMode=PictureBoxSizeMode.Zoom};
            mark.SetBounds(28,24,64,64);Controls.Add(mark);
        }
        AddText("HUNGRY GHOST AUDIO",108,22,660,40,24,true,paper);
        AddText("50 effects. One common language.",110,69,658,25,12,false,jade);
        AddText("Select your plugins. Close your audio host before installing.",28,111,740,24,10,false,paper);
        selection.SetBounds(28,147,744,298);selection.BackColor=panel;selection.ForeColor=paper;
        selection.BorderStyle=BorderStyle.FixedSingle;selection.CheckOnClick=true;selection.MultiColumn=true;selection.ColumnWidth=180;
        foreach(var bundle in Package.Bundles) selection.Items.Add(bundle=="Hungry Ghost.vst3"?"REVERB":bundle.Replace("Hungry Ghost ","").Replace(".vst3",""),true);
        // Keep the displayed names paired with the sorted, exact package names.
        Controls.Add(selection);all.Text="Select all";none.Text="Clear";all.SetBounds(28,455,105,30);none.SetBounds(141,455,105,30);
        all.Click+=(sender,e)=>SetChecks(true);none.Click+=(sender,e)=>SetChecks(false);Controls.Add(all);Controls.Add(none);
        AddText("VST3 folder",28,497,710,23,10,true,paper);
        destination.SetBounds(28,525,625,29);destination.BackColor=panel;destination.ForeColor=paper;
        destination.Text=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs","Common","VST3");Controls.Add(destination);
        browse.Text="Browse…";browse.SetBounds(662,524,110,30);browse.Click+=(sender,e)=>{using(var dialog=new FolderBrowserDialog()){dialog.Description="Choose your VST3 plugin folder";dialog.SelectedPath=destination.Text;if(dialog.ShowDialog(this)==DialogResult.OK)destination.Text=dialog.SelectedPath;}};Controls.Add(browse);
        AddText("This installs for your user. In REAPER, add this folder to the VST scan paths if needed.",28,564,744,30,9,false,paper);
        progress.SetBounds(28,607,525,18);Controls.Add(progress);status.SetBounds(28,635,744,25);status.Text="AGPLv3 source · Perpetual purchases · Two device activations";Controls.Add(status);
        install.Text="Install selected plugins";install.SetBounds(575,599,197,36);install.BackColor=jade;install.ForeColor=Color.FromArgb(12,17,17);install.FlatStyle=FlatStyle.Flat;Controls.Add(install);
        install.Click+=(sender,e)=>StartInstall();
        FormClosing+=(sender,e)=>{if(busy)e.Cancel=true;};
    }
    void AddText(string text,int x,int y,int w,int h,int size,bool bold,Color colour) { var label=new Label{Text=text,ForeColor=colour,Font=new Font("Segoe UI",size,bold?FontStyle.Bold:FontStyle.Regular)};label.SetBounds(x,y,w,h);Controls.Add(label); }
    void SetChecks(bool value){for(int i=0;i<selection.Items.Count;i++)selection.SetItemChecked(i,value);}
    void StartInstall() {
        var chosen=selection.CheckedIndices.Cast<int>().Select(index=>Package.Bundles[index]).ToArray();
        if(chosen.Length==0){status.Text="Select at least one plugin.";return;}
        string root=destination.Text;
        if(String.IsNullOrWhiteSpace(root)||!Path.IsPathRooted(root)){status.Text="Choose an absolute VST3 folder path.";return;}
        var hosts=new[]{"reaper","Live","Cubase","Nuendo","Studio One","FL64","BitwigStudio"};
        if(hosts.Any(name=>System.Diagnostics.Process.GetProcessesByName(name).Length>0)){status.Text="Close your audio host, then install again. Your projects stay untouched.";return;}
        busy=true;install.Enabled=browse.Enabled=selection.Enabled=destination.Enabled=all.Enabled=none.Enabled=false;
        status.Text="Checking package and installing…";
        Task.Factory.StartNew(()=>{
            try {
                string backup=Package.Install(root,chosen,(value,name)=>BeginInvoke((Action)(()=>{progress.Value=value;status.Text="Installed "+name;})));
                BeginInvoke((Action)(()=>{busy=false;install.Text="Installed";status.Text="Done. Rescan your host. Previous files are backed up in your user app data.";MessageBox.Show(this,"Installed "+chosen.Length+" plugins to:\r\n"+root+"\r\n\r\nOpen your audio host and rescan this folder. Enter your HG_ key using the activation button in any plugin. A new installation starts a 30-day trial.\r\n\r\nBackup: "+backup,"Hungry Ghost Audio",MessageBoxButtons.OK,MessageBoxIcon.Information);}));
            } catch(Exception error){BeginInvoke((Action)(()=>{busy=false;install.Enabled=browse.Enabled=selection.Enabled=destination.Enabled=all.Enabled=none.Enabled=true;status.Text="Installation stopped. Changed files were restored.";MessageBox.Show(this,error.Message,"Installation could not finish",MessageBoxButtons.OK,MessageBoxIcon.Error);}));}
        });
    }
}
internal static class Program {
    [STAThread] static int Main(string[] args) {
        if(args.Length>=2&&args[0]=="--render") {
            Application.EnableVisualStyles();
            using(var form=new SetupWindow())using(var image=new Bitmap(form.Width,form.Height)) {
                form.DrawToBitmap(image,new Rectangle(0,0,form.Width,form.Height));
                // Render each real native control while keeping the test
                // window hidden; WM_PRINT omits hidden child windows otherwise.
                int frameX=(form.Width-form.ClientSize.Width)/2;
                int frameY=form.Height-form.ClientSize.Height-frameX;
                using(var graphics=Graphics.FromImage(image))foreach(Control control in form.Controls) {
                    using(var child=new Bitmap(control.Width,control.Height)) {
                        control.DrawToBitmap(child,new Rectangle(0,0,child.Width,child.Height));
                        graphics.DrawImageUnscaled(child,control.Left+frameX,control.Top+frameY);
                    }
                }
                image.Save(args[1],System.Drawing.Imaging.ImageFormat.Png);
            }
            return 0;
        }
        if(args.Length>=2&&args[0]=="--verify") {try{Package.Verify();File.WriteAllText(args[1],"PASS: 50 bundles and "+Package.Hashes.Count+" embedded file hashes verified.");return 0;}catch(Exception error){File.WriteAllText(args[1],"FAIL: "+error.Message);return 1;}}
        if(args.Length>=2&&args[0]=="--extract") {try{var chosen=args.Length>=3?Package.Bundles.Where(name=>args[2].Split(',').Any(id=>name=="Hungry Ghost "+id.ToUpperInvariant()+".vst3"||(id=="reverb"&&name=="Hungry Ghost.vst3"))).ToArray():Package.Bundles;Package.Install(args[1],chosen,(value,name)=>{});return 0;}catch{return 1;}}
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Application.Run(new SetupWindow());return 0;
    }
}
