#!/bin/sh
echo "Make sure you have run: git tag -a r<maj>.<min>.<patch>"
git describe | perl -e '
	my $line = <>;	
	chomp $line;
	print "Packaging source for $line\n";
	if ($line =~ /r(\d+)\.(\d+)\.(\d+)/) {
		my $M = $1;
		my $m = $2;
		my $p = $3;
		print "Major: $M; Minor: $m; Patch: $p\n"; 

		my $prefix = "TrainGame-$M.$m.$p";
		my $file = "$prefix.tar";
		system "git archive --output=$file --prefix=$prefix/ HEAD";
		system "gzip -f $file";
	}
	else {
		die "Bad format";
	}
'
