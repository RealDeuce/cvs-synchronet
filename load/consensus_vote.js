/*
 * Hard-coded things to watch out for... voters have UFLAG_D
 * and "I don't give a shit what you hamfags do" is the string for
 * standing aside.
 * Issues expire after 15 days.
 */
load("sbbsdefs.js");
load("json-client.js");
var LOCK_READ = 1
var LOCK_WRITE = 2
var LOCK_UNLOCK = -1

function ConcensusObject(host, port, scope)
{
	var u,i,j;

	this.db=new JSONClient(host,port);
	this.scope=scope;

	this.add_issue_funcs=function(issue)
	{
		issue.new_answer=function(text)
		{
			for(i in this.answer) {
				if(this.answer[i].value == text)
					throw("Duplicate Answer Attempted!");
			}

			newanswer={};
			newanswer.value=text;
			newanswer.votes=[];
			this.answers.push(newanswer);
			return newanswer;
		}
		issue.votefor=function(answer)
		{
			var a,v;

			// Delete current vote...
			for(a=0; a<this.answers.length; a++) {
				for(v=0; v<this.answers[a].votes.length; v++) {
					if(this.answers[a].votes[v]==user.alias) {
						this.answers[a].votes.splice(v,1);
						v--;
					}
				}
				if(this.answers[a].votes.length==0) {
					this.answers.splice(a,1);
					a--;
				}
			}

			// Add new vote...
			for(a=0; a<this.answers.length; a++) {
				if(this.answers[a].value==answer) {
					this.answers[a].votes.push(user.alias);
					break;
				}
			}
			if(a==this.answers.length) {
				this.answers.push({value:answer,votes:[user.alias]});
			}

			// Write back...
			this.save();
		}
		issue.save=function()
		{
			var newissue={};
			newissue.start=this.start;
			newissue.title=this.title;
			newissue.answers=this.answers;
			this.db.write(this.scope, 'issues.'+this.index, newissue, LOCK_WRITE);
		}
		issue.myvote=function()
		{
			var i,j;

			for(i in this.answers) {
				for(j in this.answers[i].votes) {
					if(this.answers[i].votes[j]==user.alias)
						return this.answers[i].value;
				}
			}
		}
		issue.havevoted=function()
		{
			return this.myvote()!=undefined;
		}
	}

	this.get_all_issues=function()
	{
		var issues=this.db.read(this.scope, 'issues', LOCK_READ);
		var i;

		for(i in issues) {
			this.add_issue_funcs(issues[i]);
			issues[i].index=i;
			issues[i].db=this.db;
			issues[i].scope=this.scope;
		}
		return(issues);
	}

	this.get_issue=function(index)
	{
		var issue=this.db.read(this.scope, 'issues.'+index, LOCK_READ);
		this.add_issue_funcs(issue);
		issue.index=index;
		issue.db=this.db;
		issue.scope=this.scope;
		return issue;
	}	

	this.is_closed=function(issue)
	{
		var i;
		var now=time();
		var dontgiveashit;

		if(issue.start < now-60*60*24*15)
			return true;

		for(i in issue.answers) {
			if(issue.answers[i].value=="I don't give a shit what you hamfags do")
				dontgiveashit=issue.answers[i].votes.length;
		}
		for(i in issue.answers) {
			if(issue.answers[i].value!="I don't give a shit what you hamfags do") {
				if(issue.answers[i].votes.length + dontgiveashit == this.dongcount)
					return true;
			}
		}

		return false;
	}

	this.get_closed_issues=function()
	{
		var all=this.get_all_issues();
		var i;
		var ret=[];

		for (i in all) {
			if(this.is_closed(all[i]))
				ret.push(all[i]);
		}
		return ret;
	}

	this.get_open_issues=function()
	{
		var all=this.get_all_issues();
		var i;
		var ret=[];

		for (i in all) {
			if(!(this.is_closed(all[i]))) {
				ret.push(all[i]);
			}
		}
		return ret;
	}

	this.get_pending_issues=function()
	{
		var all=this.get_open_issues();
		var i;
		var ret=[];

		for(i in all) {
			if(!all[i].havevoted())
				ret.push(all[i]);
		}
		return ret;
	}

	this.create_issue=function(title)
	{
		var issue={};
		var issues,i;

		issue.start=time();
		issue.title=title;
		issue.answers=[];
		issue.index=time()+'-'+rand(1000000);
		this.db.push(this.scope, 'issues', issue, LOCK_WRITE);
		issues=this.db.read(this.scope, 'issues', LOCK_READ);
		for(i=0; i<issues.length; i++) {
			if(issues[i].index==issue.index) {
				issue=issues[i];
				issue.index=i;
			}
		}
		issue.db=this.db;
		issue.scope=this.scope;
		this.add_issue_funcs(issue);
		return issue;
	}

	this.dongcount=0;
	u=new User(1);
	for(i=1; i<system.lastuser; i++) {
		u.number=i;
		if(u.settings&USER_DELETED)
			continue;
		if(user.security.flags1 & UFLAG_D)
			this.dongcount++;
	}

	this.db.connect();
	if(this.db.read(this.scope, 'issues', LOCK_READ)==undefined)
		this.db.write(this.scope, 'issues', [], LOCK_WRITE);
}
