#! /usr/bin/perl -w
use strict;

# global variables
my $interfaceheader = "../clientinterface.h";
my $interfacename = "vandenoever.strigi";

my $class;
my %names;
my %outparams;
my $classname;

my %typemapping = (
	"std::string" => "s",
	"int" => "i",
	"bool" => "b",
	"std::vector<std::string>" => "as",
	"std::set<std::string>" => "as",
	"Hits" => "a(sdsssxxa{ss})",
	"std::map<std::string, std::string>" => "a{ss}"
);

sub parseFunction {
    my $out = shift;
    my $name = shift;
    my $in = shift;
    die "Type $out cannot be mapped." unless $typemapping{$out};
    my @args = split ',', $in;
    my @a;
    foreach (@args) {
        if (m/^\s*(.*)$/) {
            $_ = $1;
        }
        if (m/^const\s+(.*)$/) {
            $_ = $1;
        }
        my $atype;
        my $aname;
        if (m/^(.+)\&?\s+\&?(\w+)$/) {
            $atype = $1;
            $aname = $2;
        } else {
            die "PARSE ERROR: '$_'";
        }
        $atype =~ s/\&?\s*\&?$//;
        die "Type $atype cannot be mapped." unless $typemapping{$atype};
        push(@a, $atype, $aname);
    }
    $names{$name} = \@a;
}

sub printFunctionDeclaration {
    my $name = shift;
    print FH "    void $name(DBusMessage* msg, DBusConnection* conn);\n";
}

sub printFunctionDefinition {
    my $name = shift;
    print FH "void\n";
    print FH "$classname"."::$name(DBusMessage* msg, DBusConnection* conn) {\n";
    print FH "    DBusMessageReader reader(msg);\n";
    print FH "    DBusMessageWriter writer(conn, msg);\n";
    my $i = $names{$name};
    my @a = @$i;
    for ($i=1; $i < @a; $i+=2) {
        print FH "    ".$a[$i-1]." ".$a[$i].";\n";
    }
    if (@a) {
        print FH "    reader";
        for ($i=1; $i < @a; $i+=2) {
	    print FH " >> ".$a[$i];
        }
        print FH ";\n";
    }
    print FH "    if (reader.isOk()) {\n        ";
    if (length($outparams{$name}) > 0) {
        print FH "writer << ";
    }
    print FH "impl->$name(";
    for ($i=1; $i < @a; $i+=2) {
        print FH $a[$i];
        if ($i < @a-2) {
            print FH ",";
        }
    }
    print FH ");\n    }\n";
    print FH "}\n";
}

sub printIntrospectionXML {
    my $name = shift;
    my $i = $names{$name};
    my @a = @$i;
    print FH "    << \"    <method name='$name'>\\n\"\n";
    for ($i=1; $i < @a; $i+=2) {
        my $type = $typemapping{$a[$i-1]};
        print FH "    << \"      <arg name='$a[$i]' type='$type' direction='in'/>\\n\"\n";
    }
    if (length($outparams{$name}) > 0) {
        my $type = $typemapping{$outparams{$name}};
        print FH "    << \"      <arg name='out' type='$type' direction='out'/>\\n\"\n";
    }
    print FH "    << \"    </method>\\n\"\n";
}

my @lines = `cat $interfaceheader`;

# find the classname
foreach (@lines) {
    if (m/^class\s+(\w+)/) {
        $class = $1;
    }
}
die "No class found." unless defined $class;

# parse the functions from the header file
foreach (@lines) {
    # match function line
    if (m/^\s*(virtual\s+)?(.*)\s+~?(\w+)\(\s*(.*)\s*\)/) {
        if ($3 eq $class) {
            next;
        }
        if ($2 =~ m/static/) {
            next;
        }
        parseFunction($2, $3, $4);
        $outparams{$3} = $2;
    }
}

$classname = "DBus$class";
my $headerfile = lc($classname).".h";
my $cppfile = lc($classname).".cpp";

# print the binding header file
open (FH, "> $headerfile") or die;
print FH "#ifndef ".uc($classname)."_H\n";
print FH "#define ".uc($classname)."_H\n";
print FH "#include \"dbusobjectinterface.h\"\n";
print FH "#include <map>\n";
print FH "class $class;\n";
print FH "class $classname : public DBusObjectInterface {\n";
print FH "private:\n";
print FH "    typedef void (".$classname."::*handlerFunction)\n";
print FH "        (DBusMessage* msg, DBusConnection* conn);\n";
print FH "    $class* impl;\n";
print FH "    std::map<std::string, handlerFunction> handlers;\n";
print FH "    DBusHandlerResult handleCall(DBusConnection* connection, DBusMessage* msg);\n";
foreach (keys %names) {
    printFunctionDeclaration($_);
}
print FH "public:\n";
print FH "    ".$classname."(".$class."* i);\n";
print FH "    ~".$classname."() {}\n";
print FH "    std::string getIntrospectionXML();\n";
print FH "};\n";
print FH "#endif\n";
close(FH);

# print the binding implementation file
open (FH, "> $cppfile") or die;

print FH "#include \"$headerfile\"\n";
print FH "#include \"dbusmessagereader.h\"\n";
print FH "#include \"dbusmessagewriter.h\"\n";
print FH "#include \"$interfaceheader\"\n";
print FH "#include <sstream>\n";
print FH $classname."::".$classname."(".$class."* i)\n";
print FH "        :DBusObjectInterface(\"$interfacename\"), impl(i) {\n";
foreach (keys %names) {
    print FH "    handlers[\"$_\"] = &".$classname."::".$_.";\n";
}
print FH "}\n";
print FH "DBusHandlerResult\n";
print FH $classname."::handleCall(DBusConnection* connection, DBusMessage* msg){\n";
print FH <<THEEND;
    std::map<std::string, handlerFunction>::const_iterator h;
    const char* i = getInterfaceName().c_str();
    for (h = handlers.begin(); h != handlers.end(); ++h) {
        if (dbus_message_is_method_call(msg, i, h->first.c_str())) {
            (this->*h->second)(msg, connection);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
THEEND
print FH "std::string\n";
print FH $classname."::getIntrospectionXML() {\n";
print FH "    std::ostringstream xml;\n";
print FH "    xml << \"  <interface name='\"+getInterfaceName()+\"'>\\n\"\n";
foreach (keys %names) {
    printIntrospectionXML($_);
}
print FH "    << \"  </interface>\\n\";\n";
print FH "    return xml.str();\n";
print FH "}\n";
foreach (keys %names) {
    printFunctionDefinition($_);
}
close(FH);
