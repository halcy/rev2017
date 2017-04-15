use strictures;
use warnings;

use FileHandle;
use File::Basename;

sub main
{
    my ($shader_input_file, $c_header_output_file) = @_;
    my @out_lines;
    my @header_lines;

    my $header_ifndef_name = uc(basename($shader_input_file) =~ tr/./_/r);
    my $header_const_char_name = lc(basename($shader_input_file) =~ tr/./_/r);

    my $input = FileHandle->new("< $shader_input_file");
    if (defined $input) {
        while (<$input>) {
            chomp;
            next unless $_;
            push @out_lines, "\"$_\\n\"";
        }
        $input->close;
    }

    push @header_lines, "#ifndef $header_ifndef_name";
    push @header_lines, "#define $header_ifndef_name";
    push @header_lines, "const char* $header_const_char_name =";
    push @header_lines, join("\n", @out_lines) . ";";
    push @header_lines, "#endif /* $header_ifndef_name /*";

    my $output = FileHandle->new("> $c_header_output_file");
    if (defined $output) {
        print $output join("\n", @header_lines), "\n";
        $output->close;
    }
}

main @ARGV;
