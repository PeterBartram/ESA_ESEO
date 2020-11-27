require 'open3'

BUILD_SERVER_DIRECTORY = File.dirname(__FILE__);
ESEO_EMBEDDED_SOLUTION_DIRECTORY = BUILD_SERVER_DIRECTORY + '/../ESEO_Project';
UNIT_TEST_SOLUTION_DIRECTORY = BUILD_SERVER_DIRECTORY + '/TestSolution';
UNIT_TEST_PROJECTS_DIRECTORY = BUILD_SERVER_DIRECTORY + '/../TestProjects';

TEST_PROJECTS = ['DiagnosticsTest', 'DiagnosticsADCsTest']

task :default => [:BuildUnitTests, :RunUnitTests, :BuildTargetApplication] do

end

task :BuildTargetApplication do
	puts "### Building target application"
	#stdout, stderr, status = Open3.capture3("AtmelStudio.exe #{ESEO_EMBEDDED_SOLUTION_DIRECTORY}/MainApp_6_2.atsln /Rebuild Debug /project MainApp_6_2 /out buildLog.txt")
	puts File.read("buildLog.txt")
end

task :BuildUnitTests do
	puts "### Building unit tests"
	cd "#{UNIT_TEST_SOLUTION_DIRECTORY}" do
	TEST_PROJECTS.each { |project|
		puts "### Building #{project}"
		stdout, stderr, status = Open3.capture3("devenv.com  TestSolution.sln /Rebuild Release /project #{project} Release")
		puts stderr
	}
	end
end

task :RunUnitTests do
	puts "### Running unit tests"
	cd "#{UNIT_TEST_PROJECTS_DIRECTORY}" do
		TEST_PROJECTS.each { |project|
			puts "### Running #{project}"
			stdout, stderr, status = Open3.capture3("#{project}/Release/#{project}.exe --gtest_output=xml:#{project}.xml")
			puts stdout
		}
	end
end
