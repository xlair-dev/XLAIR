#include <ThirdParty/Catch2/catch.hpp>

#include <ApiClient/HttpClient.hpp>
#include "Parser.hpp"
#include <limits>

namespace api = xlair::api;

TEST_CASE("Client options validate API base URLs independently of config files", "[API][Options]") {
    api::ClientOptions options;
    for (const auto* endpoint :
         { U"https://api.example.com/v1", U"http://localhost:8080", U"https://api.example.com/" }) {
        options.endpoint = endpoint;
        CHECK_FALSE(api::ValidateClientOptions(options).has_value());
    }
    for (const auto* endpoint : {
             U"",
             U"ftp://example.com",
             U"https://",
             U"https:///v1",
             U"https://user:secret@example.com",
             U"https://example.com?x=1",
             U"https://example.com#fragment",
             U"https://example.com/a b",
         }) {
        options.endpoint = endpoint;
        const auto error = api::ValidateClientOptions(options);
        REQUIRE(error.has_value());
        CHECK(error->field == U"endpoint");
    }
}

TEST_CASE("Invalid timeouts cannot start a directly constructed HTTP client", "[API][Options]") {
    for (const double timeout : {
             0.0,
             -1.0,
             std::numeric_limits<double>::infinity(),
             std::numeric_limits<double>::quiet_NaN(),
         }) {
        api::ClientOptions options;
        options.timeout_seconds = timeout;
        const auto validation = api::ValidateClientOptions(options);
        REQUIRE(validation.has_value());
        CHECK(validation->field == U"timeout_seconds");
        api::HttpClient client{ options };
        auto request = client.fetchCatalog();
        request->update();
        REQUIRE(request->result().has_value());
        const auto* error = std::get_if<api::ApiError>(&*request->result());
        REQUIRE(error);
        CHECK(error->kind == api::ErrorKind::Configuration);
    }
}

TEST_CASE("Authentication options allow empty credentials but reject incomplete credentials", "[API][Options]") {
    api::ClientOptions options;
    CHECK_FALSE(api::ValidateClientOptions(options).has_value());
    options.auth.domain = U"tenant.auth0.com";
    const auto error = api::ValidateClientOptions(options);
    REQUIRE(error.has_value());
    CHECK(error->field == U"auth");
    options.auth.client_id = U"test-id";
    options.auth.client_secret = U"test-secret";
    CHECK_FALSE(api::ValidateClientOptions(options).has_value());
}

TEST_CASE("Record responses accept the current server shape without userId", "[API]") {
    const auto records = api::ParseRecords(s3d::JSON::Parse(UR"([
        {"id":"record-1","sheetId":"sheet-1","score":1000000,
         "clearType":"fullcombo","playCount":3,"updatedAt":"2026-09-13T00:00:00Z"}
    ])"));
    REQUIRE(records.size() == 1);
    CHECK(records[0].sheet_id == U"sheet-1");
    CHECK(records[0].clear_type == api::ClearType::FullCombo);
    CHECK(records[0].play_count == 3);
    CHECK(api::ParseRecords(s3d::JSON::Parse(U"[]")).isEmpty());
}

TEST_CASE("API integer fields reject negative, fractional and overflowing values", "[API]") {
    CHECK_THROWS_AS(api::ParseCredits(s3d::JSON::Parse(U"{\"credits\":-1}")), s3d::Error);
    CHECK_THROWS_AS(api::ParseCredits(s3d::JSON::Parse(U"{\"credits\":1.5}")), s3d::Error);
    CHECK_THROWS_AS(api::ParseCredits(s3d::JSON::Parse(U"{\"credits\":4294967296}")), s3d::Error);
    CHECK_THROWS_AS(api::ParseCredits(s3d::JSON::Parse(U"{}")), s3d::Error);
    CHECK(api::ParseCredits(s3d::JSON::Parse(U"{\"credits\":4294967295}")) == 4294967295u);
}

TEST_CASE("Invalid clear types fail rather than becoming failed plays", "[API]") {
    CHECK_THROWS_AS(
        api::ParseRecords(s3d::JSON::Parse(UR"([
        {"id":"r","sheetId":"s","score":1,"clearType":"unknown",
         "playCount":1,"updatedAt":"2026-09-13T00:00:00Z"}
    ])")),
        s3d::Error
    );
}

TEST_CASE("Submissions include the path user and preserve separate plays of the same sheet", "[API]") {
    const s3d::Array<api::RecordSubmission> records{
        { U"sheet-1", 900000, api::ClearType::Clear },
        { U"sheet-1", 1000000, api::ClearType::Perfect },
    };
    const auto json = api::SerializeRecords(U"user-1", records);
    REQUIRE(json.isArray());
    REQUIRE(json.size() == 2);
    CHECK(json[0][U"userId"].get<s3d::String>() == U"user-1");
    CHECK(json[1][U"clearType"].get<s3d::String>() == U"perfect");
    CHECK_FALSE(json[0].hasElement(U"playCount"));
    const auto empty = api::SerializeRecords(U"user-1", {});
    CHECK(empty.isArray());
    CHECK(empty.size() == 0);
}

TEST_CASE("Play option offsets retain their sign and millisecond units", "[API]") {
    const auto options = api::ParsePlayOptions(s3d::JSON::Parse(U"{\"noteSpeed\":4.5,\"judgmentOffset\":-25}"));
    CHECK(options.note_speed == Approx(4.5));
    CHECK(options.judgment_offset_ms == -25);
    CHECK(api::SerializePlayOptions(options)[U"judgmentOffset"].get<s3d::int32>() == -25);
}

TEST_CASE("A cancelled request is terminal and never starts authentication or HTTP", "[API]") {
    api::HttpClient client{ xlair::api::ClientOptions{} };
    auto request = client.findUserByCard(U"test-card");
    CHECK_FALSE(request->result().has_value());
    request->cancel();
    request->update();
    request->cancel();
    REQUIRE(request->result().has_value());
    const auto* error = std::get_if<api::ApiError>(&*request->result());
    REQUIRE(error);
    CHECK(error->kind == api::ErrorKind::Cancelled);
}

TEST_CASE("Missing supplied credentials fail without contacting an authentication server", "[API]") {
    api::HttpClient client{ api::ClientOptions{} };
    auto request = client.findUserByCard(U"test-card");
    request->update();
    REQUIRE(request->result().has_value());
    const auto* error = std::get_if<api::ApiError>(&*request->result());
    REQUIRE(error);
    CHECK(error->kind == api::ErrorKind::Configuration);
}

TEST_CASE("The client owns its authentication settings independently of the caller", "[API]") {
    api::ClientOptions options;
    options.auth = {
        .domain = U"https://invalid-hostname",
        .client_id = U"test-id",
        .client_secret = U"test-secret",
        .audience = U"test-audience",
    };
    api::HttpClient client{ options };
    options.auth = {};
    auto request = client.findUserByCard(U"test-card");
    request->update();
    REQUIRE(request->result().has_value());
    const auto* error = std::get_if<api::ApiError>(&*request->result());
    REQUIRE(error);
    CHECK(error->kind == api::ErrorKind::Configuration);
    CHECK(error->message == U"The authentication domain must be a hostname.");
}
